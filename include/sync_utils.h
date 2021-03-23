#pragma once

#include <istream>
#include <vector>
#include <string>


std::string get_strong_signature(const std::string&);
std::string get_strong_signature(std::istream&);

unsigned int get_weak_signature(
    const std::string& data, 
    unsigned long offset, 
    unsigned long block_size
);
unsigned int get_weak_signature(
    std::istream& data, 
    unsigned long offset, 
    unsigned long block_size
);

std::vector<unsigned int> get_weak_signatures(
    const std::string& data, 
    unsigned long initial_offset, 
    unsigned long block_size
);

