#pragma once

#include "file_operator/signatures.h"
#include "messages/basic.pb.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs {
    std::vector<std::filesystem::path> get_file_paths(bool include_hidden);

    bool is_hidden(const std::filesystem::path&);
    bool is_not_hidden(const std::filesystem::path&);

    std::vector<File*> get_files(const std::vector<std::filesystem::path>&);
    File* get_file(const std::filesystem::path&);

    std::vector<WeakSign> get_request_signatures(const std::filesystem::path&);
    std::vector<WeakSign> get_weak_signatures(const std::filesystem::path&);

    WeakSign get_weak_signature(
        const std::filesystem::path&,
        BlockSize = BLOCK_SIZE,
        Offset = 0
    );

    StrongSign get_strong_signature(
        const std::filesystem::path&,
        BlockSize,
        Offset
    );

    // read at given offset(s) with given size(s)
    std::vector<std::string> read(
        const std::filesystem::path&,
        const std::vector<std::pair<Offset, BlockSize>>&
    );
    std::string read(
        const std::filesystem::path&,
        Offset,
        BlockSize
    );

    // read whole file
    std::string read(const std::filesystem::path&);

    void move_file(
        const std::filesystem::path& old_path, 
        const std::filesystem::path& new_path
    );

    void remove_file(const std::filesystem::path&);
}
