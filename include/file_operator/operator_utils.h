#pragma once

#include "file_operator/signatures.h"
#include "messages/all.pb.h"

#include <string>
#include <utility>
#include <vector>


std::vector<Block*> get_blocks_between(
    std::vector<Offset>&& offsets,
    size_t file_size,
    const std::string& file_name,
    BlockSize block_size = BLOCK_SIZE
);

std::vector<std::pair<Offset, BlockSize>> get_blocks_between(
    std::vector<Offset>&& offsets,
    size_t file_size,
    BlockSize block_size = BLOCK_SIZE
);
