#include "file_operations.h"
#include "message_utils.h"
#include "sync_utils.h"
#include "utils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <ios>

using namespace std;
using namespace filesystem;

Block* get_block(const string& file_name, unsigned long offset = 0);


ShowFiles* get_show_files(QueryOptions* options) {
    auto show_files{new ShowFiles};

    show_files->set_allocated_options(options);

    return show_files;
}

FileList* get_file_list(QueryOptions* options, const vector<File*>& files) {
    auto file_list{new FileList};

    for (auto file: files) {
        if ((options->include_hidden() 
                || 
             is_not_hidden(file->file_name())
            ) && (
             !options->has_timestamp() 
                || 
             file->timestamp() > options->timestamp()
        )) {
            auto new_file{file_list->add_files()};
            new_file->set_file_name(file->file_name());
            new_file->set_timestamp(file->timestamp());
            new_file->set_size(file->size());
            new_file->set_signature(file->signature());
        }
    }

    file_list->set_allocated_options(options);

    return file_list;
}


QueryOptions* get_query_options(
    bool include_hidden, 
    optional<chrono::time_point<chrono::system_clock>> changed_after
) {
    return 
        get_query_options(
            include_hidden, 
            changed_after.has_value()
            ? optional{get_timestamp(changed_after.value())}
            : nullopt
        );
}

QueryOptions* get_query_options(
    bool include_hidden, 
    std::optional<unsigned long> changed_after
) {
    auto query_options{new QueryOptions};

    query_options->set_include_hidden(include_hidden);

    if (changed_after.has_value()) {
        query_options->set_timestamp(changed_after.value());
    }

    return query_options;
}


SyncRequest* get_sync_request(File* file) {
    auto request{new SyncRequest};

    request->set_allocated_file(file);
    
    ifstream file_stream{file->file_name(), ios::binary};
    auto signatures{
        get_weak_signatures(
            file_stream, 
            file_size(file->file_name()), 
            file->size()
        )};
    for (auto signature: signatures) {
        request->add_weak_signatures(signature);
    }

    return request;
}


FileRequest* get_file_request(File* file) {
    auto request{new FileRequest};

    request->set_allocated_file(file);

    return request;
}

FileResponse* get_file_response(const FileRequest& request) {
    auto response{new FileResponse};

    response->set_allocated_requested_file(new File(request.file()));
    response->set_data("");

    return response;
}


Block* get_block(const string& file_name, unsigned long offset) {
    auto block{new Block};

    block->set_file_name(file_name);
    block->set_offset(offset);
    block->set_size(6000);

    return block;
}
