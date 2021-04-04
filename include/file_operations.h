#pragma once

#include "messages/all.pb.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>


std::vector<std::filesystem::path> get_file_paths(bool include_hidden);

bool is_hidden(const std::filesystem::path&);
bool is_not_hidden(const std::filesystem::path&);

std::vector<File*> get_files(const std::vector<std::filesystem::path>&);
File* get_file(const std::filesystem::path&);

void move_file(const std::string& old_name, const std::string& new_name);

void remove_file(const std::string& file_name);

std::vector<File*> to_vector(const std::unordered_map<std::string, File*>&);
