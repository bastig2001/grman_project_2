#pragma once

#include "file_operator/signatures.h"
#include "type/definitions.h"
#include "messages/all.pb.h"

#include <chrono>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>


// used ... object is used and get disposed by Protobuf
// copied ... object data is copied to new object, 
//            caller remains owner of the passed in object

// creational functions for basic message types

File* file(
    const FileName& name,
    Timestamp timestamp,
    size_t size,
    const StrongSign& signature
);

Block* block(
    const FileName& file_name, 
    Offset offset = 0, 
    BlockSize size = BLOCK_SIZE
);

Blocks* blocks(const std::vector<Block* /* used */>&);


// creational functions for download message types

FileRequest* file_request(const File&);

FileResponse* file_response(
    const File& requested_file, 
    std::variant<std::string, bool>&& response
);


// creational functions for info message types

QueryOptions* query_options(
    bool include_hidden, 
    std::optional<std::chrono::time_point<std::chrono::system_clock>> 
        changed_after
);
QueryOptions* query_options(
    bool include_hidden, 
    std::optional<Timestamp> changed_after
);

ShowFiles* show_files(QueryOptions* /* used */);

FileList* file_list(
    const std::vector<File* /* copied */>&, 
    QueryOptions* /* used */
);


// creational functions for sync message types

Correction* correction(Block* /* used */, std::string&& data);

Corrections* corrections(const std::vector<Correction* /* used */>&);

BlockWithSignature* block_with_signature(
    Block* /* used */, 
    const StrongSign& strong_signature
);

PartialMatch* partial_match(
    const File&, 
    std::optional<Blocks* /* used */> signature_requests = std::nullopt,
    std::optional<Corrections* /* used */> = std::nullopt
);

SyncRequest* sync_request(
    const File&,
    const std::vector<WeakSign>& weak_signatures,
    bool removed = false
);

SyncResponse* sync_response(
    const File& requested_file,
    std::optional<PartialMatch* /* used */>,
    std::optional<Blocks* /* used */> correction_request,
    bool requesting_file = false,
    bool removed = false
);

SignatureAddendum* signature_addendum(
    const File& matched_file, 
    const std::vector<BlockWithSignature* /* used */>&
);


// all

Message received();


// other utils

std::vector<File*> to_vector(
    const std::unordered_map<FileName, File* /* not copied */>&
);

std::vector<std::pair<Offset, BlockSize>> get_block_positioners(const Blocks&);
std::pair<Offset, BlockSize> get_block_positioners(const Block&);
