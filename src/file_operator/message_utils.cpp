#include "file_operator/message_utils.h"
#include "messages/sync.pb.h"
#include "utils.h"
#include "messages/all.pb.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace std;


// creational functions for basic message types

File* file(
    const string& file_name,
    chrono::time_point<chrono::system_clock> timestamp,
    unsigned long size,
    const string& signature
) {
    return file(file_name, get_timestamp(timestamp), size, signature);
}

File* file(
    const string& file_name,
    unsigned long timestamp,
    unsigned long size,
    const string& signature
) {
    auto file{new File};
    file->set_file_name(file_name);
    file->set_timestamp(timestamp);
    file->set_size(size);
    file->set_signature(signature);

    return file;
}

Block* block(
    const string& file_name, 
    unsigned long offset, 
    unsigned int size
) {
    auto block{new Block};
    block->set_file_name(file_name);
    block->set_offset(offset);
    block->set_size(size);

    return block;
}

Blocks* blocks(const vector<Block* /* used */>& block_vector) {
    auto blocks{new Blocks};

    for (Block* block: block_vector) {
        blocks->mutable_blocks()->AddAllocated(block);
    }

    return blocks;
}


// creational functions for download message types

FileRequest* file_request(const File& file) {
    auto request{new FileRequest};
    request->set_allocated_file(new File(file));

    return request;
}

FileResponse* file_response(
    const File& requested_file, 
    variant<string, bool>&& response
) {
    auto file_response{new FileResponse};
    file_response->set_allocated_requested_file(new File(requested_file));

    if (auto data{get_if<string>(&response)}) {
        file_response->set_data(move(*data));
    }
    else {
        file_response->set_unknown(get<bool>(response));
    }

    return file_response;
}


// creational functions for info message types

QueryOptions* query_options(
    bool include_hidden, 
    optional<chrono::time_point<chrono::system_clock>> changed_after
) {
    return 
        query_options(
            include_hidden, 
            changed_after.has_value()
            ? optional{get_timestamp(changed_after.value())}
            : nullopt
        );
}

QueryOptions* query_options(
    bool include_hidden, 
    optional<unsigned long> changed_after
) {
    auto query_options{new QueryOptions};

    query_options->set_include_hidden(include_hidden);

    if (changed_after.has_value()) {
        query_options->set_timestamp(changed_after.value());
    }

    return query_options;
}

ShowFiles* show_files(QueryOptions* /* used */ options) {
    auto show_files{new ShowFiles};
    show_files->set_allocated_options(options);

    return show_files;
}

FileList* file_list(
    const vector<File* /* copied */>& files, 
    QueryOptions* /* used */ options
) {
    auto file_list{new FileList};

    for (File* file: files) {
        file_list->mutable_files()->AddAllocated(new File(*file));
    }

    file_list->set_allocated_options(options);

    return file_list;
}


// creational functions for sync message types

Correction* correction(Block* /* used */ block, string&& data) {
    auto correction{new Correction};
    correction->set_allocated_block(block);
    correction->set_data(move(data));

    return correction;
}

Corrections* corrections(const vector<Correction* /* used */>& correction_vector) {
    auto corrections{new Corrections};

    for (Correction* correction: correction_vector) {
        corrections->mutable_corrections()->AddAllocated(correction);
    }

    return corrections;
}

BlockWithSignature* block_with_signature(
    Block* /* used */ block, 
    const string& strong_signature
) {
    auto block_with_signature{new BlockWithSignature};
    block_with_signature->set_allocated_block(block);
    block_with_signature->set_strong_signature(strong_signature);

    return block_with_signature;
}

PartialMatch* partial_match(
    const File& matched_file, 
    optional<Blocks* /* used */> signature_requests,
    optional<Corrections* /* used */> corrections
) {
    auto partial_match{new PartialMatch};
    partial_match->set_allocated_matched_file(new File(matched_file));

    if (signature_requests.has_value()) {
        partial_match->set_allocated_signature_requests(
            signature_requests.value()
        );
    }

    if (corrections.has_value()) {
        partial_match->set_allocated_corrections(corrections.value());
    }

    return partial_match;
}

SyncRequest* sync_request(
    const File& file,
    const vector<unsigned int>& weak_signatures,
    bool removed
) {
    auto request{new SyncRequest};
    request->set_allocated_file(new File(file));

    for (unsigned int signature: weak_signatures) {
        request->add_weak_signatures(signature);
    }

    request->set_removed(removed);

    return request;
}

SyncResponse* sync_response(
    const File& requested_file,
    optional<PartialMatch* /* used */> partial_match,
    optional<Blocks* /* used */> correction_request,
    bool requesting_file,
    bool removed
) {
    auto response{new SyncResponse};
    response->set_allocated_requested_file(new File(requested_file));

    if (partial_match.has_value()) {
        response->set_allocated_partial_match(partial_match.value());
    }

    if (correction_request.has_value()) {
        response->set_allocated_correction_request(correction_request.value());
    }

    response->set_requsting_file(requesting_file);
    response->set_removed(removed);

    return response;
}

SignatureAddendum* signature_addendum(
    const File& matched_file, 
    const vector<BlockWithSignature* /* used */>& blocks_with_signature
) {
    auto addendum{new SignatureAddendum};
    addendum->set_allocated_matched_file(new File(matched_file));

    for (BlockWithSignature* block: blocks_with_signature) {
        addendum->mutable_blocks_with_signature()->AddAllocated(block);
    }

    return addendum;
}


// other utils

vector<File*> to_vector(
    const unordered_map<string, File* /* not copied */>& file_map
) {
    vector<File*> files(file_map.size());
    transform(
        file_map.begin(), 
        file_map.end(), 
        files.begin(), 
        [](auto key_file){ return key_file.second; }
    );

    return files;
}
