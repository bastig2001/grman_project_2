#pragma once

#include "file_operator/signatures.h"
#include "messages/all.pb.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


std::vector<std::filesystem::path> get_file_paths(bool include_hidden);

bool is_hidden(const std::filesystem::path&);
bool is_not_hidden(const std::filesystem::path&);

std::vector<File*> get_files(const std::vector<std::filesystem::path>&);
File* get_file(const std::filesystem::path&);

std::vector<unsigned int> get_request_signatures(const std::filesystem::path&);
std::vector<unsigned int> get_weak_signatures(const std::filesystem::path&);
unsigned int get_weak_signature(
    const std::filesystem::path&,
    BlockSize = BLOCK_SIZE,
    Offset = 0
);

std::vector<std::string> read(
    const std::filesystem::path&,
    const std::vector<std::pair<Offset, BlockSize>>&
);
std::string read(
    const std::filesystem::path&,
    Offset,
    BlockSize
);

void move_file(
    const std::filesystem::path& old_path, 
    const std::filesystem::path& new_path
);

void remove_file(const std::filesystem::path&);
