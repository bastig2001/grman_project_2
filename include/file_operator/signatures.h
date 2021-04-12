#pragma once

#include "type/definitions.h"

#include <istream>
#include <vector>
#include <string>


const BlockSize BLOCK_SIZE{6000};


// returns the strong signature (MD5) of the given data
StrongSign get_strong_signature(const std::string&);
StrongSign get_strong_signature(std::istream&);

// returns the weak signature of the specified data
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

// returns the weak signatures at all offsets for the given data 
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

