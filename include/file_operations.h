#pragma once

#include "messages/basic.pb.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <filesystem>
#include <string>
#include <vector>


std::vector<std::filesystem::path> get_files(bool include_hidden);

ShowFiles* get_show_files(const std::vector<std::string>& = {});
FileList* show_files(const ShowFiles&);

SyncRequest* get_sync_request(Block*);
SyncResponse* get_sync_response(const SyncRequest&);

CheckFileRequest* get_check_file_request(File*);
CheckFileResponse* get_check_file_response(const CheckFileRequest&);

FileRequest* get_file_request(File*);
FileResponse* get_file_response(const FileRequest&);

File* get_file(const std::filesystem::path&);
