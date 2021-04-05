#include "file_operator/message_utils.h"
#include "file_operator/filesystem.h"
#include "file_operator/signatures.h"
#include "utils.h"

#include <google/protobuf/repeated_field.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ios>
#include <unordered_map>

using namespace std;
using namespace filesystem;

unordered_map<unsigned int, unsigned int> signatures_to_hashmap(
    const google::protobuf::RepeatedField<unsigned int>
);
Block* get_block(const string& file_name, unsigned long offset = 0);


ShowFiles* get_show_files(QueryOptions* options) {
    auto show_files{new ShowFiles};

    show_files->set_allocated_options(options);

    return show_files;
}

FileList* get_file_list(QueryOptions* options, const vector<File*>& files) {
    auto file_list{new FileList};

    for (auto file: files) {
        if ((options->include_hidden() 
                || 
             is_not_hidden(file->file_name())
            ) && (
             !options->has_timestamp() 
                || 
             file->timestamp() > options->timestamp()
        )) {
            auto new_file{file_list->add_files()};
            new_file->set_file_name(file->file_name());
            new_file->set_timestamp(file->timestamp());
            new_file->set_size(file->size());
            new_file->set_signature(file->signature());
        }
    }

    file_list->set_allocated_options(options);

    return file_list;
}


QueryOptions* get_query_options(
    bool include_hidden, 
    optional<chrono::time_point<chrono::system_clock>> changed_after
) {
    return 
        get_query_options(
            include_hidden, 
            changed_after.has_value()
            ? optional{get_timestamp(changed_after.value())}
            : nullopt
        );
}

QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<unsigned long> changed_after
) {
    auto query_options{new QueryOptions};

    query_options->set_include_hidden(include_hidden);

    if (changed_after.has_value()) {
        query_options->set_timestamp(changed_after.value());
    }

    return query_options;
}


SyncRequest* get_sync_request(File* file, bool removed) {
    auto request{new SyncRequest};

    request->set_allocated_file(file);
    request->set_removed(removed);
    
    if (!removed) {
        ifstream file_stream{file->file_name(), ios::binary};
        auto signatures{
            get_weak_signatures(
                file_stream, 
                file_size(file->file_name()), 
                file->size()
            )};
        for (auto signature: signatures) {
            request->add_weak_signatures(signature);
        }
    }

    return request;
}

PartialMatch* get_partial_match_with_corrections(
    const SyncRequest&, 
    const File& file
) {
    auto match{new PartialMatch};
    match->set_allocated_matched_file(new File(file));

    return match;
}

PartialMatch* get_partial_match_without_corrections(
    const SyncRequest& request, 
    const File& file
) {
    auto match{new PartialMatch};
    match->set_allocated_matched_file(new File(file));

    auto received{signatures_to_hashmap(request.weak_signatures())};
    auto local{get_weak_signatures(file.file_name())};
    vector<pair<unsigned long, unsigned long>> matching_offsets{};

    for (unsigned long i{0}; i < local.size();) {
        if (contains(received, local[i])) {
            // matching signatures have been found
            unsigned long received_offset{received[local[i]] * block_size};
            matching_offsets.push_back({i, received_offset});
            
            i += block_size;
        }
        else {
            i++;
        }
    }

    return match;
}

Blocks* get_correction_request(
    const PartialMatch&, 
    const File&
) {
    auto request{new Blocks};

    return request;
}


FileRequest* get_file_request(File* file) {
    auto request{new FileRequest};

    request->set_allocated_file(file);

    return request;
}

FileResponse* get_file_response(const FileRequest& request) {
    auto response{new FileResponse};

    response->set_allocated_requested_file(new File(request.file()));
    response->set_data("");

    return response;
}


unordered_map<unsigned int, unsigned int> signatures_to_hashmap(
    const google::protobuf::RepeatedField<unsigned int> signatures
) {
    unordered_map<unsigned int, unsigned int> result{};

    for (int i{0}; i < signatures.size(); i++) {
        result.insert({signatures.at(i), i});
    }

    return result;
}

Block* get_block(const string& file_name, unsigned long offset) {
    auto block{new Block};

    block->set_file_name(file_name);
    block->set_offset(offset);
    block->set_size(block_size);

    return block;
}
