#pragma once

#include <vector>
#include <string>


std::string get_md5_hash(const std::string&);

unsigned int get_rolling_checksum(
    const std::string& data, 
    unsigned long offset, 
    unsigned long block_size
);

std::vector<unsigned int> get_rolling_checksums(
    const std::string& data, 
    unsigned long initial_offset, 
    unsigned long block_size
);

