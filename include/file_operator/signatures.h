#pragma once

#include <istream>
#include <vector>
#include <string>

using Offset = unsigned long;
using BlockSize = unsigned int;

const BlockSize BLOCK_SIZE{6000};


std::string get_strong_signature(const std::string&);
std::string get_strong_signature(std::istream&);

unsigned int get_weak_signature(
    const std::string& data,  
    BlockSize block_size = BLOCK_SIZE,
    Offset offset = 0
);
unsigned int get_weak_signature(
    std::istream& data,  
    BlockSize block_size = BLOCK_SIZE,
    Offset offset = 0
);

std::vector<unsigned int> get_weak_signatures(
    const std::string& data,  
    BlockSize block_size = BLOCK_SIZE,
    Offset initial_offset = 0
);
std::vector<unsigned int> get_weak_signatures(
    std::istream& data, 
    size_t data_size,
    BlockSize block_size = BLOCK_SIZE,
    Offset initial_offset = 0
);

