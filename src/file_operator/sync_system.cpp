#include "file_operator/sync_system.h"
#include "file_operator/filesystem.h"
#include "file_operator/operator_utils.h"
#include "file_operator/signatures.h"
#include "file_operator/sync_utils.h"
#include "config.h"
#include "database.h"
#include "message_utils.h"
#include "utils.h"
#include "messages/basic.h"
#include "presentation/format_utils.h"
#include "presentation/logger.h"
#include "type/definitions.h"
#include "type/result.h"
#include "type/sequence.h"
#include "messages/all.pb.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;


SyncSystem::SyncSystem(const Config& config): config{config} {
    auto file_paths{get_file_paths(config)};

    logger->debug("Files to sync:\n" + vector_to_string(file_paths, "\n"));

    if (!filesystem::exists(".sync/")) {
        filesystem::create_directory(".sync/");
    }

    db::create(filesystem::exists(".sync/db.sqlite"));
    db::insert_files(get_files(move(file_paths)));
}


void SyncSystem::check_filesystem() {
    auto new_files{get_files(get_file_paths(config))};
    auto old_files{db::get_files()};

    unordered_map<FileName, msg::File> old_files_by_name;
    old_files_by_name.reserve(old_files.size());
    
    for (auto file: old_files) {
        old_files_by_name.insert({file.name, move(file)});
    }

    for (auto file: new_files) {
        if (contains(old_files_by_name, file.name)) {
            logger->debug(file.name + " still exists");

            old_files_by_name.erase(file.name);
            db::update_file(move(file));
        }
        else {
            logger->debug(file.name + " is new");

            db::insert_file(move(file));
        }
    }

    for (auto [name, remaining_file]: old_files_by_name) {
        logger->debug(name + " was removed");

        db::insert_removed(remaining_file);
        db::delete_file(name);
    }
}


Message SyncSystem::get_show_files() {
    Message msg{};
    msg.set_allocated_show_files(
        show_files(
            query_options(
                config.sync.sync_hidden_files,
                db::get_last_checked()
    )));

    db::insert_or_update_last_checked(
        get_timestamp(
            cast_clock<chrono::time_point<filesystem::file_time_type::clock>>(
                chrono::system_clock::now()
    )));

    return msg;
}


Message SyncSystem::get_file_list(const ShowFiles& request) {
    bool list_hidden{
        config.sync.sync_hidden_files && request.options().include_hidden()
    };
    optional<Timestamp> min_timestamp{
        request.options().has_timestamp()
        ? optional{request.options().timestamp()}
        : nullopt
    };

    auto listed_files{
        Sequence(db::get_files())
        .where([&](const msg::File& file){
            return (   
                list_hidden 
                    || 
                fs::is_not_hidden(file.name)
            ) && (
                !min_timestamp.has_value() 
                    ||
                min_timestamp.value() <= file.timestamp
            );
        })
        .map<File*>([](msg::File file){
            return file.to_proto();
        })
        .to_vector()
    };

    Message response{};
    response.set_allocated_file_list(
        file_list(listed_files, query_options(list_hidden, min_timestamp))
    );

    return response;
}


vector<Message> SyncSystem::get_sync_requests(const FileList& server_list) {
    vector<Message> msgs{};
    vector<FileName> checked_files{};

    for (auto& server_file: server_list.files()) {
        if (auto file_result{db::get_file(server_file.name())}) {
            // locally there is a file with the same name/relative path

            auto local_file{file_result.get_ok()};
            checked_files.push_back(local_file.name);

            if (!(local_file.timestamp == server_file.timestamp() 
                    && 
                  local_file.size == server_file.size()
                    &&
                  local_file.signature == server_file.signature()
            )) {
                // local file and server file are not equal
                
                start_sync(move(local_file))
                .apply(
                    [&](Message msg){ msgs.push_back(msg); },
                    [&](Error err){ logger->error(err.msg); }
                );
            } 
            else {
                // local file and server file seem to be equal

                logger->info(colored(local_file) + " needs no syncing");
            }
        }
        else if (auto file_result{db::get_removed(server_file.name())}) {
            // locally there was a file with the same name/relative path,
            // it has been removed

            auto removed_file{file_result.get_ok()};

            if (removed_file.timestamp >= server_file.timestamp()) {
                msgs.push_back(notify_already_removed(server_file));
            }
            else {
                // file seems to be newer

                db::delete_removed(server_file.name());
                msgs.push_back((request(server_file)));
            }
            
        }
        else {
            // file seems to be unknown

            msgs.push_back(request(server_file));
        }
    }

    for (auto file: db::get_files()) {
        if (!contains(checked_files, file.name)
            && (
                server_list.options().include_hidden() 
                || 
                fs::is_not_hidden(file.name)
            ) && (
                !server_list.options().has_timestamp()
                ||
                server_list.options().timestamp() <= file.timestamp
            )
        ) {
            // server doesn't seem to know of this file

            start_sync(move(file))
            .apply(
                [&](Message msg){ msgs.push_back(msg); },
                [&](Error err){ logger->error(err.msg); }
            );
        }
    }

    return msgs;
}

Result<Message> SyncSystem::start_sync(msg::File file) {
    logger->info("Starting syncing process for " + colored(file));

    return
        fs::get_request_signatures(file.name)
        .map<Message>([&](vector<WeakSign> signatures){
            Message msg{};
            msg.set_allocated_sync_request(
                sync_request(file.to_proto(), signatures) 
            );
            
            return msg;
        });
}

Message SyncSystem::notify_already_removed(const File& file) {
    logger->info(
        colored(msg::File::from_proto(file)) + " has already been removed"
    );

    Message msg{};
    msg.set_allocated_sync_request(sync_request(new File(file), {}, true));

    return msg;
}

Message SyncSystem::request(const File& file) {
    logger->info("Requesting " + colored(msg::File::from_proto(file)));

    Message msg{};
    msg.set_allocated_file_request(file_request(file));

    return msg;
}


Message SyncSystem::get_sync_response(const SyncRequest& request) {
    auto client_file{request.file()};

    if (request.removed()) {
        if (auto file{db::get_file(client_file.name())}) {
            remove(file.get_ok().name);
        }

        return received();
    }
    else if (auto file{db::get_file(client_file.name())}) {
        return sync(request, file.get_ok()).or_else(received());
    }
    else if (auto removed{db::get_removed(client_file.name())}) {
        return respond_already_removed(client_file);
    }
    else {
        return respond_requesting(client_file);
    }
}

Result<Message> SyncSystem::sync(
    const SyncRequest& request, 
    msg::File local_file
) {
    auto client_file{request.file()};
    BlockSize last_block_size{(BlockSize)(client_file.size() % BLOCK_SIZE)};
    bool last_block_smaller{last_block_size < BLOCK_SIZE};

    logger->info("Syncing " + colored(local_file) + " with client");

    int full_signatures_count{
        request.weak_signatures_size() + (
            last_block_smaller
            ? -1 // the last signature in the request is not from a full block
            :  0 
    )};

    unordered_map<unsigned int, vector<Offset>> signature_offsets{};
    for (int i{0}; i <  full_signatures_count; i++) {
        auto signature{request.weak_signatures().at(i)};

        if (contains(signature_offsets, signature)) {
            signature_offsets[signature].push_back(i * BLOCK_SIZE);
        }
        else {
            signature_offsets.insert({signature, {i * BLOCK_SIZE}});
        }
    }

    return
    fs::get_weak_signatures(local_file.name)
    .flat_map<vector<BlockPair*>>([&](vector<WeakSign> signatures){
        vector<pair<Offset /* client */, Offset /* local */>> matching_offsets{};
        Offset client_offset{0};
        for (unsigned long i{0}; i < signatures.size();) {

            if (contains(signature_offsets, signatures[i])) {
                while (
                    signature_offsets[signatures[i]].size() > 0
                        &&
                    client_offset > signature_offsets[signatures[i]][0]
                ) {
                    signature_offsets[signatures[i]].erase(
                        signature_offsets[signatures[i]].begin()
                    );
                }

                if (signature_offsets[signatures[i]].size() > 0) {
                    client_offset = signature_offsets[signatures[i]][0];
                    signature_offsets[signatures[i]].erase(
                        signature_offsets[signatures[i]].begin()
                    );

                    matching_offsets.push_back({client_offset, i});
                    i += BLOCK_SIZE;
                }
                else {
                    i++;
                }
            }
            else {
                i++;
            }
        }

        vector<BlockPair*> matching_blocks{};
        matching_blocks.reserve(matching_offsets.size());

        for (auto [client_offset, local_offset]: matching_offsets) {
            matching_blocks.push_back(
                block_pair(client_file.name(), client_offset, local_offset)
            );
        }

        if (last_block_smaller && request.weak_signatures_size() >= 1) {
            auto last_offset{local_file.size - last_block_size};

            return
                fs::get_weak_signature(
                    local_file.name, 
                    last_block_size, 
                    last_offset
                )
                .map<vector<BlockPair*>>([&](WeakSign last_signature){
                    if (last_signature 
                        == 
                        request.weak_signatures()
                            .at(request.weak_signatures_size() - 1)
                    ) {
                        auto last_client_offset{client_file.size() - last_block_size};

                        matching_blocks.push_back(
                            block_pair(
                                client_file.name(), 
                                last_client_offset, 
                                last_offset,
                                last_block_size
                        ));
                    }

                    return matching_blocks;
                });
        }
        else {
            return Result<vector<BlockPair*>>::ok(matching_blocks);
        }
    })
    .map<pair<vector<BlockPair*> /* matching */, vector<BlockPair*> /* not matching */>>(
    [&](vector<BlockPair*> matching){
        return pair{
            matching, 
            get_block_pairs_between(
                matching, 
                local_file.name, 
                client_file.size(), 
                local_file.size
            )
        };
    })
    .map<Message>([&](pair<vector<BlockPair*>, vector<BlockPair*>> pairs){
        auto [matching, non_matching]{pairs};
        Message msg{};

        if (client_file.timestamp() > local_file.timestamp) {
            // client file is newer

            msg.set_allocated_sync_response(sync_response(
                client_file,
                partial_match(
                    local_file.to_proto(),
                    block_pairs(move(matching))
                ),
                block_pairs(non_matching)
            ));
        }
        else {
            // server file is newer

            msg.set_allocated_sync_response(sync_response(
                client_file,
                partial_match(
                    local_file.to_proto(),
                    block_pairs(matching),
                    get_corrections(
                        move(non_matching), 
                        client_file.name(),
                        matching.size() == 0
                    )
                ),
                nullopt
            ));
        }

        return msg;
    });
}

Message SyncSystem::respond_already_removed(const File& requested_file) {
    logger->info(
        "Requested " + 
        colored(msg::File::from_proto(requested_file)) + 
        " has already been removed"
    );

    Message msg{};
    msg.set_allocated_sync_response(
        sync_response(requested_file, nullopt, nullopt, false, true)
    );

    return msg;
}

Message SyncSystem::respond_requesting(const File& requested_file) {
    logger->info(
        "Getting " + 
        colored(msg::File::from_proto(requested_file)) + 
        " from client"
    );

    Message msg{};
    msg.set_allocated_sync_response(
        sync_response(requested_file, nullopt, nullopt, true)
    );

    return msg;
}


vector<Message> SyncSystem::handle_sync_response(const SyncResponse& response) {
    auto file{response.requested_file()};

    if (response.requsting_file()) {
        return {get_file(response.requested_file())};
    }
    else if (response.removed()) {
        remove(file.name());

        return {received()};
    }
    else if (response.has_partial_match()) {
        return sync(response);
    }
    else {
        return {received()};
    }
}

vector<Message> SyncSystem::sync(const SyncResponse& response) {
    auto file{response.requested_file()};
    auto match{response.partial_match()};
    vector<Message> msgs;

    bool has_signature_requests{
        match.has_signature_requests() 
            && 
        match.signature_requests().block_pairs_size() > 0
    };

    if (has_signature_requests) {
        auto signatures{
            Sequence(vector(
                match.signature_requests().block_pairs().begin(),
                match.signature_requests().block_pairs().end()
            ))
            .map<Result<BlockWithSignature*>>([](BlockPair pair){
                return
                fs::get_strong_signature(
                    pair.file_name(),
                    pair.offset_client(),
                    pair.size_client()
                )
                .map<BlockWithSignature*>([&](StrongSign signature){
                    return block_with_signature(
                        new BlockPair(pair),
                        signature
                    );
                });
            })
            .peek([](Result<BlockWithSignature*> result){
                result.peek(
                    [](auto){},
                    [&](Error err){ logger->error(err.msg); }
                );
            })
            .where([](Result<BlockWithSignature*> result){
                return result.is_ok();
            })
            .map<BlockWithSignature*>([](Result<BlockWithSignature*> result){
                return result.get_ok();
            })
            .to_vector()
        };

        Message msg{};
        msg.set_allocated_signature_addendum(
            signature_addendum(file, move(signatures))
        );

        msgs.push_back(move(msg));
    }

    if (match.has_corrections()) {
        correct(match.corrections());
    }
    else if (response.has_correction_request()
            &&
            response.correction_request().block_pairs_size() > 0
    ) {
        Message msg{};
        msg.set_allocated_corrections(get_corrections(
            Sequence(vector(
                response.correction_request().block_pairs().begin(),
                response.correction_request().block_pairs().end()
            ))
            .map<BlockPair*>([](BlockPair pair){
                return new BlockPair(pair);
            })
            .to_vector(),
            file.name(),
            !has_signature_requests
        ));

        msgs.push_back(move(msg));
    }

    return 
        msgs.size() > 0
        ? msgs
        : vector{received()};
}

Message SyncSystem::correct(const Corrections& corrections) {
    if (corrections.corrections_size() > 0) {
        db::insert_data(
            Sequence(vector(
                corrections.corrections().begin(),
                corrections.corrections().end()
            ))
            .map<msg::Data>([](Correction correction){
                return msg::Data(correction);
            })
            .to_vector()
        );
    }

    if(corrections.final()) {
        ::correct(corrections.file_name());
    }

    return received();
}


Message SyncSystem::get_sync_response(const SignatureAddendum& addendum) {
    auto client_file{addendum.matched_file()};

    return
    db::get_file(client_file.name())
    .map<Message>([&](msg::File local_file){
        vector<BlockPair*> matching{};
        vector<BlockPair*> non_matching{};

        for (auto block_with_signature: addendum.blocks_with_signature()) {
            auto block_pair{new BlockPair(block_with_signature.block())};
            auto signature{block_with_signature.strong_signature()};

            bool match{
                fs::get_strong_signature(
                    block_pair->file_name(),
                    block_pair->size_server(),
                    block_pair->offset_server()
                )
                .map<bool>([&](StrongSign local_signature){
                    return local_signature == signature;
                })
                .peek(
                    [](auto){},
                    [&](Error err){ logger->error(err.msg); }
                )
                .or_else(false)
            };

            if (match) {
                matching.push_back(block_pair);
            }
            else {
                non_matching.push_back(block_pair);
            }
        }

        Message msg{};

        if (client_file.timestamp() > local_file.timestamp) {
            // client file is newer

            msg.set_allocated_sync_response(sync_response(
                client_file,
                partial_match(local_file.to_proto()),
                block_pairs(non_matching)
            ));
        }
        else {
            // server file is newer

            msg.set_allocated_sync_response(sync_response(
                client_file,
                partial_match(
                    local_file.to_proto(),
                    nullopt,
                    get_corrections(
                        move(non_matching), 
                        client_file.name(),
                        true
                    )
                ),
                nullopt
            ));
        }

        return msg;
    })
    .or_else(received());
}


Message SyncSystem::get_file(const File& file) {
    return
        db::get_file(file.name())
        .flat_map<string>([](msg::File file){
            return fs::read(file.name);
        })
        .map<Message>([&](string data){
            Message msg{};
            msg.set_allocated_file_response(
                file_response(file, move(data))
            );

            return msg;
        })
        .or_else([&](){
            Message msg{};
            msg.set_allocated_file_response(
                file_response(file, true)
            );

            return msg;
        }());
}


Message SyncSystem::create_file(const FileResponse& response) {
    auto file{msg::File::from_proto(response.requested_file())};

    logger->info("Got " + colored(file));
    
    file.timestamp = 
        get_timestamp(
            cast_clock<chrono::time_point<filesystem::file_time_type::clock>>(
                chrono::system_clock::now()
            ));

    db::insert_file(file);

    fs::write(file.name, string{response.data()})
    .apply(
        [](auto){},
        [&](Error err){
            logger->error(err.msg);
        }
    );
    
    return received();
}
