#pragma once

#include <istream>
#include <vector>
#include <string>


std::string get_strong_signature(const std::string&);
std::string get_strong_signature(std::istream&);

unsigned int get_weak_signature(
    const std::string& data,  
    unsigned long block_size,
    unsigned long offset = 0
);
unsigned int get_weak_signature(
    std::istream& data,  
    unsigned long block_size,
    unsigned long offset = 0
);

std::vector<unsigned int> get_weak_signatures(
    const std::string& data,  
    unsigned long block_size,
    unsigned long initial_offset = 0
);
std::vector<unsigned int> get_weak_signatures(
    std::istream& data, 
    size_t data_size,
    unsigned long block_size,
    unsigned long initial_offset = 0
);

