#pragma once

#include "file_operator/signatures.h"
#include "messages/basic.h"
#include "type/result.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs {
    std::vector<std::filesystem::path> get_file_paths(bool include_hidden);

    bool is_hidden(const std::filesystem::path&);
    bool is_not_hidden(const std::filesystem::path&);

    std::vector<Result<msg::File>> get_files(const std::vector<std::filesystem::path>&);
    Result<msg::File> get_file(const std::filesystem::path&);

    Result<std::vector<WeakSign>> get_request_signatures(const std::filesystem::path&);
    Result<std::vector<WeakSign>> get_weak_signatures(const std::filesystem::path&);

    Result<WeakSign> get_weak_signature(
        const std::filesystem::path&,
        BlockSize = BLOCK_SIZE,
        Offset = 0
    );

    Result<StrongSign> get_strong_signature(
        const std::filesystem::path&,
        BlockSize,
        Offset
    );

    // read at given offset(s) with given size(s)
    Result<std::vector<std::string>> read(
        const std::filesystem::path&,
        const std::vector<std::pair<Offset, BlockSize>>&
    );
    Result<std::string> read(
        const std::filesystem::path&,
        Offset,
        BlockSize
    );

    // read whole file
    Result<std::string> read(const std::filesystem::path&);

    Result<bool> move_file(
        const std::filesystem::path& old_path, 
        const std::filesystem::path& new_path
    );

    void remove_file(const std::filesystem::path&);
}
