#pragma once

#include "messages/all.pb.h"

#include <chrono>
#include <optional>
#include <vector>


ShowFiles* get_show_files(QueryOptions*);
FileList* get_file_list(QueryOptions*, const std::vector<File*>&);

QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<std::chrono::time_point<std::chrono::system_clock>> 
        changed_after
);
QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<unsigned long> changed_after
);

SyncRequest* get_sync_request(File* file, bool removed = false);

PartialMatch* get_partial_match_with_corrections(
    const SyncRequest&, 
    const File&
);
PartialMatch* get_partial_match_without_corrections(
    const SyncRequest&, 
    const File&
);

Blocks* get_correction_request(const PartialMatch&, const File&);

FileRequest* get_file_request(File*);
FileResponse* get_file_response(const FileRequest&);
