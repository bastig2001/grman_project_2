#pragma once

#include "file_operator/signatures.h"
#include "messages/all.pb.h"

#include <string>
#include <vector>


std::vector<Block*> get_blocks_between(
    std::vector<unsigned long>&& offsets,
    size_t file_size,
    const std::string& file_name,
    unsigned int block_size = BLOCK_SIZE
);
