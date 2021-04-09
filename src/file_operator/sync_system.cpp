#include "file_operator/sync_system.h"
#include "file_operator/filesystem.h"
#include "file_operator/message_utils.h"
#include "file_operator/operator_utils.h"
#include "file_operator/signatures.h"
#include "config.h"
#include "utils.h"
#include "messages/all.pb.h"
#include "presentation/format_utils.h"
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
        if (!contains(checked_files, name)
            && (
                server_list.options().include_hidden() 
                || 
                fs::is_not_hidden(name)
            )) {
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

        return received();
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

    if (local_file->timestamp() < client_file.timestamp()) {
        // server has older file and requests correction

        waiting_for_sync.insert({local_file->name(), local_file});

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


vector<Message> SyncSystem::handle_sync_response(const SyncResponse& response) {
    auto file{response.requested_file()};

    if (response.requsting_file()) {
        waiting_for_sync.erase(waiting_for_sync.find(file.name()));

        return {get_file(response.requested_file())};
        
    }
    else if (response.removed()) {
        if (contains(files, file.name())) {
            waiting_for_sync.erase(waiting_for_sync.find(file.name()));
            remove(files[file.name()]);
        }

        return {received()};
    }
    else if (response.has_partial_match() 
                && 
             contains(waiting_for_sync, file.name())
    ) {
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

    if (match.has_signature_requests() 
            && 
        match.signature_requests().blocks_size() > 0
    ) {
        vector<BlockWithSignature*> blocks_with_signature{};
        blocks_with_signature.reserve(match.signature_requests().blocks_size());

        for (auto block: match.signature_requests().blocks()) {
            blocks_with_signature.push_back(
                block_with_signature(
                    new Block(block), 
                    fs::get_strong_signature(
                        file.name(), 
                        block.size(), 
                        block.offset()
            )));
        }

        Message msg{};
        msg.set_allocated_signature_addendum(
            signature_addendum(match.matched_file(), blocks_with_signature)
        );

        msgs.push_back(move(msg));

        if (match.has_corrections()
                &&
            match.corrections().corrections_size() > 0
        ) {
            postponed_corrections.insert(
                {file.name(), match.release_corrections()}
            );
        }
    }
    else {
        waiting_for_sync.erase(file.name());

        if (match.has_corrections()
                &&
            match.corrections().corrections_size() > 0
        ) {
            correct(file, match.corrections());
        }
    }

    
    if (!(match.has_corrections()
            &&
          match.corrections().corrections_size() > 0
        ) && (
        response.has_correction_request()
            &&
        response.correction_request().blocks_size() > 0
    )) {
        auto blocks_to_read{
            get_block_positioners(response.correction_request())
        };

        auto correction_data{fs::read(file.name(), blocks_to_read)};

        vector<Correction*> corrections{};
        corrections.reserve(correction_data.size());
        
        for (unsigned int i{0}; i < correction_data.size(); i++) {
            corrections.push_back(
                correction(
                    new Block(response.correction_request().blocks().at(i)), 
                    move(correction_data[i])
            ));
        }

        Message msg{};
        msg.set_allocated_corrections(
            ::corrections(corrections)
        );

        msgs.push_back(msg);
    }

    return msgs;
}

void SyncSystem::correct(const File& file, const Corrections&) {
    logger->info("Correcting " + colored(file));
}


Message SyncSystem::get_sync_response(const SignatureAddendum& addendum) {
    auto file{addendum.matched_file()};

    if (contains(files, file.name())) {
        auto& matching_offsets{this->matching_offsets[file.name()]};
        vector<pair<Offset, BlockSize>> blocks_to_read;
        vector<Block*> blocks_to_correct;

        for (auto block_with_signature: addendum.blocks_with_signature()) {
            auto block{block_with_signature.block()};
            auto signature{block_with_signature.strong_signature()};
            auto local_offset{matching_offsets[block.offset()]};

            if (signature 
                != 
                fs::get_strong_signature(
                    file.name(), 
                    block.size(), 
                    local_offset
            )) {
                blocks_to_read.push_back({local_offset, block.size()});
                blocks_to_correct.push_back(new Block(block));
            }
        }

        if (contains(waiting_for_sync, file.name())) {
            // server has older file and requests correction data

            Message msg{};
            msg.set_allocated_sync_response(
                sync_response(
                    file,
                    nullopt,
                    blocks(blocks_to_correct)
                )
            );

            return msg;
        }
        else {
            // server has newer file and sends client correction data

            auto correction_data{fs::read(file.name(), blocks_to_read)};
            vector<Correction*> corrections{};
            corrections.reserve(correction_data.size());

            for (unsigned int i{0}; i < correction_data.size(); i++) {
                corrections.push_back(
                    ::correction(
                        blocks_to_correct[i],
                        move(correction_data[i])
                ));
            }

            Message msg{};
            msg.set_allocated_sync_response(
                sync_response(
                    file, 
                    partial_match(file, nullopt, ::corrections(corrections)), 
                    nullopt
            ));

            return msg;
        }
    }
    else {
        return received();
    }
}


void SyncSystem::remove(File* file) {
    logger->info("Removing " + colored(*file));

    removed.insert({file->name(), file});
    files.erase(file->name());
    waiting_for_sync.erase(waiting_for_sync.find(file->name()));
    matching_offsets.erase(matching_offsets.find(file->name()));
    postponed_corrections.erase(postponed_corrections.find(file->name()));

    fs::remove_file(file->name());
}

Message SyncSystem::get_file(const File& file) {
    Message msg{};

    if (contains(files, file.name())) {
        msg.set_allocated_file_response(
            file_response(file, fs::read(file.name()))
        );
    }
    else {
        msg.set_allocated_file_response(
            file_response(file, true)
        );
    }
    
    return msg;
}
