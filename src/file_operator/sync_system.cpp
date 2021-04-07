#include "file_operator/sync_system.h"
#include "file_operator/filesystem.h"
#include "file_operator/message_utils.h"
#include "file_operator/signatures.h"
#include "config.h"
#include "messages/sync.pb.h"
#include "utils.h"
#include "presentation/logger.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;


SyncSystem::SyncSystem(const SyncConfig& config): config{config} {
    auto file_paths = get_file_paths(config.sync_hidden_files);
    logger->debug("Files to sync:\n" + vector_to_string(file_paths, "\n"));

    for (auto file: get_files(file_paths)) {
        files.insert({file->file_name(), file});
    }
}

SyncSystem::~SyncSystem() {
    // this includes also all files in waiting_for_sync
    for (auto [file_name, file]: files) {
        delete file;
    }

    for (auto [file_name, file]: removed) {
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
    for (auto [file_name, file]: files) {
        if ((   list_hidden 
                || 
                is_not_hidden(file_name)
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
    vector<string> checked_file_names{};

    for (auto& server_file: server_list.files()) {
        if (contains(files, server_file.file_name())) {
            // locally there is a file with the same name/relative path

            auto local_file{files[server_file.file_name()]};
            checked_file_names.push_back(local_file->file_name());

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
        else if (contains(removed, server_file.file_name())) {
            // locally there was a file with the same name/relative path,
            // it has been removed

            msgs.push_back(notify_already_removed(server_file));
        }
        else {
            // file seems to be unknown

            msgs.push_back(request(server_file));
        }
    }

    for (auto [file_name, file]: files) {
        if (!contains(checked_file_names, file_name)) {
            // server doesn't seem to know of this file

            msgs.push_back(start_sync(file));
        }
    }

    return msgs;
}

Message SyncSystem::start_sync(File* file) {
    logger->info("Starting syncing process for " + colored(*file));

    waiting_for_sync.insert({file->file_name(), file});

    Message msg{};
    msg.set_allocated_sync_request(
        sync_request(*file, get_request_signatures(file->file_name()))
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
    Message msg{};
    auto client_file{request.file()};

    if (request.removed()) {
        if (contains(files, client_file.file_name())) {
            remove(files[client_file.file_name()]);
        }

        msg.set_received(true);
    }
    else if (contains(files, client_file.file_name())) {
        sync(request);
    }
    else if (contains(removed, client_file.file_name())) {
        respond_already_removed(client_file);
    }
    else {
        respond_requesting(client_file);
    }
    
    return msg;
}

void SyncSystem::remove(File* file) {
    logger->info("Removing " + colored(*file));

    removed.insert({file->file_name(), file});
    files.erase(file->file_name());

    remove_file(file->file_name());
}

Message SyncSystem::sync(const SyncRequest& request) {
    auto client_file{request.file()};
    auto local_file{files[client_file.file_name()]};
    unsigned int last_block_size{
        BLOCK_SIZE - (unsigned int)(client_file.size() % BLOCK_SIZE)
    };
    bool last_block_smaller{last_block_size < BLOCK_SIZE};

    logger->info("Syncing " + colored(*local_file) + " with client");

    int full_signatures_count{
        request.weak_signatures_size() + (
            last_block_smaller
            ? -1 // the last signature in the request is not from a full block
            :  0 
    )};
    unordered_map<unsigned int, unsigned long> signature_offsets{};
    for (int i{0}; i <  full_signatures_count; i++) {
        signature_offsets.insert(
            {request.weak_signatures().at(i), i * BLOCK_SIZE}
        );
    }

    auto signatures{get_weak_signatures(local_file->file_name())};
    auto& matching_offsets{this->matching_offsets[local_file->file_name()]};
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
            block(client_file.file_name(), client_offset)
        );
    }

    // bool last_blocks_match{false};

    if (last_block_smaller) {
        auto last_offset{local_file->size() - last_block_size};
        auto last_signature{
            get_weak_signature(
                local_file->file_name(), 
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
                    client_file.file_name(), 
                    last_client_offset, 
                    last_block_size
            ));
        }
    }


    Message msg{};

    if (local_file->timestamp() < client_file.timestamp()) {
        // server has older file and requests correction

        
    }
    else {
        // server has newer file and sends correction
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
