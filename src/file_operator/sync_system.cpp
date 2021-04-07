#include "file_operator/sync_system.h"
#include "file_operator/filesystem.h"
#include "file_operator/message_utils.h"
#include "file_operator/operator_utils.h"
#include "file_operator/signatures.h"
#include "config.h"
#include "utils.h"
#include "messages/sync.pb.h"
#include "presentation/logger.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;


SyncSystem::SyncSystem(const SyncConfig& config): config{config} {
    auto file_paths = fs::get_file_paths(config.sync_hidden_files);
    logger->debug("Files to sync:\n" + vector_to_string(file_paths, "\n"));

    for (auto file: fs::get_files(file_paths)) {
        files.insert({file->name(), file});
    }
}

SyncSystem::~SyncSystem() {
    // this includes also all files in waiting_for_sync
    for (auto [name, file]: files) {
        delete file;
    }

    for (auto [name, file]: removed) {
        delete file;
    }
}


Message SyncSystem::get_show_files() {
    Message msg{};
    msg.set_allocated_show_files(
        show_files(
            query_options(
                config.sync_hidden_files,
                last_checked
    )));

    return msg;
}


Message SyncSystem::get_file_list(const ShowFiles& request) {
    bool list_hidden{
        config.sync_hidden_files && request.options().include_hidden()
    };
    optional<unsigned long> min_timestamp{
        request.options().has_timestamp()
        ? optional{request.options().timestamp()}
        : nullopt
    };

    vector<File*> listed_files{};
    for (auto [name, file]: files) {
        if ((   list_hidden 
                || 
                fs::is_not_hidden(name)
            ) && (
                !min_timestamp.has_value() 
                ||
                min_timestamp.value() <= file->timestamp()
        )) {
            // file can be listed
            listed_files.push_back(file);
        }
    }

    Message response{};
    response.set_allocated_file_list(
        file_list(listed_files, query_options(list_hidden, min_timestamp))
    );

    return response;
}


vector<Message> SyncSystem::get_sync_requests(const FileList& server_list) {
    vector<Message> msgs{};
    vector<string> checked_files{};

    for (auto& server_file: server_list.files()) {
        if (contains(files, server_file.name())) {
            // locally there is a file with the same name/relative path

            auto local_file{files[server_file.name()]};
            checked_files.push_back(local_file->name());

            if (!(local_file->timestamp() == server_file.timestamp() 
                    && 
                  local_file->size() == server_file.size()
                    &&
                  local_file->signature() == server_file.signature()
            )) {
                // local file and server file are not equal
                
                msgs.push_back(start_sync(local_file));
            } 
            else {
                // local file and server file seem to be equal

                logger->info(colored(*local_file) + " needs no syncing");
            }
        }
        else if (contains(removed, server_file.name())) {
            // locally there was a file with the same name/relative path,
            // it has been removed

            msgs.push_back(notify_already_removed(server_file));
        }
        else {
            // file seems to be unknown

            msgs.push_back(request(server_file));
        }
    }

    for (auto [name, file]: files) {
        if (!contains(checked_files, name)) {
            // server doesn't seem to know of this file

            msgs.push_back(start_sync(file));
        }
    }

    return msgs;
}

Message SyncSystem::start_sync(File* file) {
    logger->info("Starting syncing process for " + colored(*file));

    waiting_for_sync.insert({file->name(), file});

    Message msg{};
    msg.set_allocated_sync_request(
        sync_request(*file, fs::get_request_signatures(file->name()))
    );
    
    return msg;
}

Message SyncSystem::notify_already_removed(const File& file) {
    logger->info(colored(file) + " has already been removed");

    Message msg{};
    msg.set_allocated_sync_request(sync_request(file, {}, true));

    return msg;
}

Message SyncSystem::request(const File& file) {
    logger->info("Requesting " + colored(file));

    Message msg{};
    msg.set_allocated_file_request(file_request(file));

    return msg;
}


Message SyncSystem::get_sync_response(const SyncRequest& request) {
    auto client_file{request.file()};

    if (request.removed()) {
        if (contains(files, client_file.name())) {
            remove(files[client_file.name()]);
        }

        Message msg{};
        msg.set_received(true);

        return msg;
    }
    else if (contains(files, client_file.name())) {
        return sync(request);
    }
    else if (contains(removed, client_file.name())) {
        return respond_already_removed(client_file);
    }
    else {
        return respond_requesting(client_file);
    }
}

void SyncSystem::remove(File* file) {
    logger->info("Removing " + colored(*file));

    removed.insert({file->name(), file});
    files.erase(file->name());

    fs::remove_file(file->name());
}

Message SyncSystem::sync(const SyncRequest& request) {
    auto client_file{request.file()};
    auto local_file{files[client_file.name()]};
    BlockSize last_block_size{(BlockSize)(client_file.size() % BLOCK_SIZE)};
    bool last_block_smaller{last_block_size < BLOCK_SIZE};

    logger->info("Syncing " + colored(*local_file) + " with client");

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

    auto signatures{fs::get_weak_signatures(local_file->name())};
    auto& matching_offsets{this->matching_offsets[local_file->name()]};
    for (unsigned long i{0}; i < signatures.size();) {
        if (contains(signature_offsets, signatures[i])) {
            matching_offsets.insert({i, signature_offsets[signatures[i]]});
            i += BLOCK_SIZE;
        }
        else {
            i++;
        }
    }

    vector<Block*> matching_client_blocks{};
    matching_client_blocks.reserve(matching_offsets.size());

    for (auto [local_offset, client_offset]: matching_offsets) {
        matching_client_blocks.push_back(
            block(client_file.name(), client_offset)
        );
    }

    // bool last_blocks_match{false};

    if (last_block_smaller) {
        auto last_offset{local_file->size() - last_block_size};
        auto last_signature{
            fs::get_weak_signature(
                local_file->name(), 
                last_block_size, 
                last_offset
        )};

        if (last_signature 
            == 
            request.weak_signatures().at(request.weak_signatures_size() - 1)
        ) {
            // last_blocks_match = true;

            auto last_client_offset{client_file.size() - last_block_size};

            matching_offsets.insert({last_offset, last_client_offset});
            matching_client_blocks.push_back(
                block(
                    client_file.name(), 
                    last_client_offset, 
                    last_block_size
            ));
        }
    }


    Message msg{};

    if (local_file->timestamp() < client_file.timestamp()) {
        // server has older file and requests correction

        vector<Offset> client_offsets(matching_offsets.size());
        transform(
            matching_offsets.begin(),
            matching_offsets.end(),
            client_offsets.begin(),
            [](auto matching_pair){
                return matching_pair.second;
            }
        );

        msg.set_allocated_sync_response(
            sync_response(
                client_file,
                partial_match(
                    *local_file,
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
                return matching_pair.second;
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
                return matching_pair.first;
            }
        );
 
        auto blocks_to_read{
            get_blocks_between(move(local_offsets), local_file->size())
        };
        auto correction_data{fs::read(local_file->name(), blocks_to_read)};

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
                    *local_file,
                    blocks(matching_client_blocks),
                    ::corrections(corrections)
                ),
                nullopt
            )
        );
    }

    return msg;
}

Message SyncSystem::respond_already_removed(const File& requested_file) {
    logger->info(
        "Requested " + colored(requested_file) + " has already been removed"
    );

    Message msg{};
    msg.set_allocated_sync_response(
        sync_response(requested_file, nullopt, nullopt, false, true)
    );

    return msg;
}

Message SyncSystem::respond_requesting(const File& requested_file) {
    logger->info("Getting " + colored(requested_file) + " from client");

    Message msg{};
    msg.set_allocated_sync_response(
        sync_response(requested_file, nullopt, nullopt, true)
    );

    return msg;
}
