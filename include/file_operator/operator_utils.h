#pragma once

#include "file_operator/signatures.h"
#include "messages/all.pb.h"

#include <string>
#include <utility>
#include <vector>


std::vector<BlockPair*> get_block_pairs_between(
    std::vector<BlockPair*>& matching,
    const FileName&,
    size_t client_file_size,
    size_t server_file_size
);
