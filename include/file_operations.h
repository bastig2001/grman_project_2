#pragma once

#include "messages/basic.pb.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>


std::vector<std::filesystem::path> get_files(bool include_hidden);

ShowFiles* get_show_files(QueryOptions*);

QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<std::chrono::time_point<std::chrono::system_clock>> changed_after
);

SyncRequest* get_sync_request(Block*);

CheckFileRequest* get_check_file_request(File*);
CheckFileResponse* get_check_file_response(const CheckFileRequest&);

FileRequest* get_file_request(File*);
FileResponse* get_file_response(const FileRequest&);

File* get_file(const std::filesystem::path&);

bool is_hidden(const std::filesystem::path&);
bool is_not_hidden(const std::filesystem::path&);
