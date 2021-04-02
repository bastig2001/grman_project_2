#pragma once

#include "messages/basic.pb.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


std::vector<std::filesystem::path> get_files(bool include_hidden);

ShowFiles* get_show_files(QueryOptions*);
FileList* get_file_list(QueryOptions*, const std::vector<File*>&);

QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<std::chrono::time_point<std::chrono::system_clock>> changed_after
);
QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<unsigned long> changed_after
);

SyncRequest* get_sync_request(Block*);

CheckFileRequest* get_check_file_request(File*);
CheckFileResponse* get_check_file_response(const CheckFileRequest&);

FileRequest* get_file_request(File*);
FileResponse* get_file_response(const FileRequest&);

File* get_file(const std::filesystem::path&);

bool is_hidden(const std::filesystem::path&);
bool is_not_hidden(const std::filesystem::path&);

std::vector<File*> to_vector(const std::unordered_map<std::string, File*>&);
