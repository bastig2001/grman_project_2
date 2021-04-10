#pragma once

#include <istream>
#include <vector>
#include <string>

using Offset = unsigned long;
using BlockSize = unsigned int;
using WeakSign = unsigned int;
using StrongSign = std::string;

const BlockSize BLOCK_SIZE{6000};


StrongSign get_strong_signature(const std::string&);
StrongSign get_strong_signature(std::istream&);

WeakSign get_weak_signature(
    const std::string& data,  
    BlockSize block_size = BLOCK_SIZE,
    Offset offset = 0
);
WeakSign get_weak_signature(
    std::istream& data,  
    BlockSize block_size = BLOCK_SIZE,
    Offset offset = 0
);

std::vector<WeakSign> get_weak_signatures(
    const std::string& data,  
    BlockSize block_size = BLOCK_SIZE,
    Offset initial_offset = 0
);
std::vector<WeakSign> get_weak_signatures(
    std::istream& data, 
    size_t data_size,
    BlockSize block_size = BLOCK_SIZE,
    Offset initial_offset = 0
);

