#include "file_operations.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

using namespace std;


ShowFiles* get_show_files(const vector<string>& options) {
    auto show_files{new ShowFiles};

    for (auto option : options) {
        show_files->add_options(option);
    }

    return show_files;
}

FileList* show_files(const ShowFiles&) {
    auto file_list{new FileList};
    return file_list;
}

SyncRequest* get_sync_request(Block* block) {
    auto request{new SyncRequest};

    request->set_allocated_starting_block(block);

    return request;
}

SyncResponse* get_sync_response(const SyncRequest&) {
    auto response{new SyncResponse};

    response->set_full_match(true);

    return response;
}

CheckFileResponse* get_check_file_response(const CheckFileRequest& request) {
    auto response{new CheckFileResponse};

    response->set_allocated_requested_file(new File(request.file()));
    response->set_match(true);

    return response;
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
