#include "message_utils.h"
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
    const FileName& name,
    Timestamp timestamp,
    size_t size,
    const StrongSign& signature
) {
    auto file{new File};
    file->set_name(name);
    file->set_timestamp(timestamp);
    file->set_size(size);
    file->set_signature(signature);

    return file;
}

Block* block(
    const FileName& file_name, 
    Offset offset, 
    BlockSize size
) {
    auto block{new Block};
    block->set_file_name(file_name);
    block->set_offset(offset);
    block->set_size(size);

    return block;
}

BlockPair* block_pair(
    const FileName& file_name, 
    Offset offset_client,
    Offset offset_server, 
    BlockSize size
) {
    auto block_pair{new BlockPair};
    block_pair->set_file_name(file_name);
    block_pair->set_offset_client(offset_client);
    block_pair->set_offset_server(offset_server);
    block_pair->set_size_client(size);
    block_pair->set_size_server(size);

    return block_pair;
}

BlockPair* block_pair(
    const FileName& file_name, 
    Offset offset_client,
    Offset offset_server, 
    BlockSize size_client,
    BlockSize size_server
) {
    auto block_pair{new BlockPair};
    block_pair->set_file_name(file_name);
    block_pair->set_offset_client(offset_client);
    block_pair->set_offset_server(offset_server);
    block_pair->set_size_client(size_client);
    block_pair->set_size_server(size_server);

    return block_pair;
}

BlockPairs* block_pairs(const vector<BlockPair* /* used */>& block_vector) {
    auto blocks{new BlockPairs};

    for (BlockPair* block_pair: block_vector) {
        blocks->mutable_block_pairs()->AddAllocated(block_pair);
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
            ? optional{get_timestamp(
                cast_clock<chrono::time_point<filesystem::file_time_type::clock>>(
                    changed_after.value()
              ))}
            : nullopt
        );
}

QueryOptions* query_options(
    bool include_hidden, 
    optional<Timestamp> changed_after
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

Corrections* corrections(
    const vector<Correction* /* used */>& correction_vector,
    bool final
) {
    auto corrections{new Corrections};
    corrections->set_final(final);

    for (Correction* correction: correction_vector) {
        corrections->mutable_corrections()->AddAllocated(correction);
    }

    return corrections;
}

BlockWithSignature* block_with_signature(
    BlockPair* /* used */ block_pair, 
    const StrongSign& strong_signature
) {
    auto block_with_signature{new BlockWithSignature};
    block_with_signature->set_allocated_block(block_pair);
    block_with_signature->set_strong_signature(strong_signature);

    return block_with_signature;
}

PartialMatch* partial_match(
    File* /* used */ matched_file, 
    optional<BlockPairs* /* used */> signature_requests,
    optional<Corrections* /* used */> corrections
) {
    auto partial_match{new PartialMatch};
    partial_match->set_allocated_matched_file(matched_file);

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
    File* /* used */ file,
    const vector<WeakSign>& weak_signatures,
    bool removed
) {
    auto request{new SyncRequest};
    request->set_allocated_file(file);

    for (unsigned int signature: weak_signatures) {
        request->add_weak_signatures(signature);
    }

    request->set_removed(removed);

    return request;
}

SyncResponse* sync_response(
    const File& requested_file,
    optional<PartialMatch* /* used */> partial_match,
    optional<BlockPairs* /* used */> correction_request,
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


// all

Message received() {
    Message msg{};
    msg.set_received(true);

    return msg;
}


// other utils

vector<File*> to_vector(
    const unordered_map<FileName, File* /* not copied */>& file_map
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

vector<pair<Offset, BlockSize>> get_block_positioners(
    const vector<Block>& blocks
) {
    vector<pair<Offset, BlockSize>> positioners{};
    positioners.reserve(blocks.size());

    for (auto block: blocks) {
        positioners.push_back(get_block_positioners(block));
    }

    return positioners;
}

pair<Offset, BlockSize> get_block_positioners(const Block& block) {
    return {block.offset(), block.size()};
}
