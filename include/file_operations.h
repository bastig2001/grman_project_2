#pragma once

#include "messages/basic.pb.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <string>
#include <vector>


ShowFiles* get_show_files(const std::vector<std::string>& = {});

FileList* show_files(const ShowFiles&);

SyncRequest* get_sync_request(Block*);

SyncResponse* get_sync_response(const SyncRequest&);

GetRequest* get_get_request(Block*);

GetRequest* get_get_request(File*);

GetResponse* get_get_response(const GetRequest&);
