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
    unordered_map<unsigned int, Offset> signature_offsets{};
    for (int i{0}; i <  full_signatures_count; i++) {
        signature_offsets.insert(
            {request.weak_signatures().at(i), i * BLOCK_SIZE}
        );
    }

    if (auto signatures_result{fs::get_weak_signatures(local_file.name)}) {
        auto signatures{signatures_result.get_ok()};
        unordered_map<Offset, Offset> matching_offsets{};
        for (unsigned long i{0}; i < signatures.size();) {
            if (contains(signature_offsets, signatures[i])) {
                matching_offsets.insert({signature_offsets[signatures[i]], i});
                i += BLOCK_SIZE;
            }
            else {
                i++;
            }
        }

        vector<Block*> matching_client_blocks{};
        matching_client_blocks.reserve(matching_offsets.size());

        for (auto [client_offset, local_offset]: matching_offsets) {
            matching_client_blocks.push_back(
                block(client_file.name(), client_offset)
            );
        }

        if (last_block_smaller) {
            auto last_offset{local_file.size - last_block_size};
            auto last_signature{
                fs::get_weak_signature(
                    local_file.name, 
                    last_block_size, 
                    last_offset
            ).get_ok()};

            if (last_signature 
                == 
                request.weak_signatures().at(request.weak_signatures_size() - 1)
            ) {
                auto last_client_offset{client_file.size() - last_block_size};

                matching_offsets.insert({last_client_offset, last_offset});
                matching_client_blocks.push_back(
                    block(
                        client_file.name(), 
                        last_client_offset, 
                        last_block_size
                ));
            }
        }


        Message msg{};

        if (local_file.timestamp < client_file.timestamp()) {
            // server has older file and requests correction

            vector<Offset> client_offsets(matching_offsets.size());
            transform(
                matching_offsets.begin(),
                matching_offsets.end(),
                client_offsets.begin(),
                [](auto matching_pair){
                    return matching_pair.first;
                }
            );

            msg.set_allocated_sync_response(
                sync_response(
                    client_file,
                    partial_match(
                        local_file.to_proto(),
                        blocks(matching_client_blocks)
                    ),
                    blocks(
                        get_blocks_between(
                            move(client_offsets), 
                            client_file.size(), 
                            client_file.name()
                    ))
                )
            );
        }
        else {
            // server has newer file and sends correction

            vector<Offset> client_offsets(matching_offsets.size());
            transform(
                matching_offsets.begin(),
                matching_offsets.end(),
                client_offsets.begin(),
                [](auto matching_pair){
                    return matching_pair.first;
                }
            );

            auto blocks_to_correct{
                get_blocks_between(
                    move(client_offsets), 
                    client_file.size(), 
                    client_file.name()
            )};

            vector<Offset> local_offsets(matching_offsets.size());
            transform(
                matching_offsets.begin(),
                matching_offsets.end(),
                local_offsets.begin(),
                [](auto matching_pair){
                    return matching_pair.second;
                }
            );
    
            auto blocks_to_read{
                get_blocks_between(move(local_offsets), local_file.size)
            };
            auto correction_data{fs::read(local_file.name, blocks_to_read).get_ok()};

            vector<Correction*> corrections{};
            corrections.reserve(correction_data.size());
            
            for (unsigned int i{0}; i < correction_data.size(); i++) {
                corrections.push_back(
                    correction(blocks_to_correct[i], move(correction_data[i]))
                );
            }

            msg.set_allocated_sync_response(
                sync_response(
                    client_file,
                    partial_match(
                        local_file.to_proto(),
                        blocks(matching_client_blocks),
                        ::corrections(corrections)
                    ),
                    nullopt
                )
            );
        }

        return Result<Message>::ok(msg);
    }
    else {
        return Result<Message>::err(signatures_result.get_err());
    }
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
