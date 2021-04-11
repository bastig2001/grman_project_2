#include "file_operator/sync_system.h"
#include "file_operator/filesystem.h"
#include "file_operator/operator_utils.h"
#include "file_operator/signatures.h"
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


SyncSystem::SyncSystem(const SyncConfig& config): config{config} {
    auto file_paths = fs::get_file_paths(config.sync_hidden_files);
    logger->debug("Files to sync:\n" + vector_to_string(file_paths, "\n"));

    db::create();
    db::insert_or_replace_files(
        Sequence(fs::get_files(file_paths))
        .peek([](Result<msg::File> file){
            file.peek(
                [](auto){},
                [](Error err){
                    logger->error(err.msg);
                }
            );
        })
        .where([](const Result<msg::File>& file){
            return file.is_ok();
        })
        .map<msg::File>([](Result<msg::File> file){
            return file.get_ok();
        })
        .to_vector()
    );
}


Message SyncSystem::get_show_files() {
    Message msg{};
    msg.set_allocated_show_files(
        show_files(
            query_options(
                config.sync_hidden_files,
                db::get_last_checked()
    )));

    return msg;
}


Message SyncSystem::get_file_list(const ShowFiles& request) {
    bool list_hidden{
        config.sync_hidden_files && request.options().include_hidden()
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
            )) {
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

        if (last_block_smaller) {
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

            auto corrections{
                Sequence(move(non_matching))
                .map<Result<Correction*>>([](BlockPair* pair){
                    return
                    fs::read(
                        pair->file_name(), 
                        pair->offset_server(), 
                        pair->size_server()
                    )
                    .peek(
                        [](auto){},
                        [&](Error err){ logger->error(err.msg); }
                    )
                    .map<Correction*>([&](string data){
                        return ::correction(
                            block(
                                pair->file_name(),
                                pair->offset_client(),
                                pair->size_client()
                            ),
                            move(data)
                        );
                    });
                })
                .where([](Result<Correction*> result){
                    return result.is_ok();
                })
                .map<Correction*>([](Result<Correction*> result){ 
                    return result.get_ok(); 
                })
                .to_vector()
            };

            msg.set_allocated_sync_response(sync_response(
                client_file,
                partial_match(
                    local_file.to_proto(),
                    block_pairs(move(matching)),
                    ::corrections(move(corrections))
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


vector<Message> SyncSystem::handle_sync_response(const SyncResponse& /*response*/) {
    // auto file{response.requested_file()};

    return {};
}

vector<Message> SyncSystem::sync(const SyncResponse& /*response*/) {
    return {};
}

void SyncSystem::correct(const File& file, const Corrections&) {
    logger->info("Correcting " + colored(msg::File::from_proto(file)));
}


Message SyncSystem::get_sync_response(const SignatureAddendum& /*addendum*/) {
    //auto file{addendum.matched_file()};

    return received();
}


void SyncSystem::remove(const FileName& file) {
    logger->info("Removing " + colored(file));

    db::get_file(file)
    .apply(
        [](msg::File file){
            db::insert_or_replace_removed(file);
            db::delete_file(file.name);
            fs::remove_file(file.name);
        }
    );
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
    file.timestamp = 
        get_timestamp(
            cast_clock<chrono::time_point<filesystem::file_time_type::clock>>(
                chrono::system_clock::now()
            ));

    db::insert_or_replace_file(file);

    fs::write(file.name, string{response.data()})
    .apply(
        [](auto){},
        [&](Error err){
            logger->error(err.msg);
        }
    );
    
    return received();
}
