#include "file_operations.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

using namespace std;


ShowFiles* get_show_files(const std::vector<std::string>& options) {
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

    request->set_allocated_block(block);
    request->set_strong_signature("");
    request->set_weak_signature("");

    return request;
}

SyncResponse* get_sync_response(const SyncRequest& request) {
    auto response{new SyncResponse};

    response->set_allocated_block(new Block(request.block()));
    response->set_match(false);

    return response;
}

GetRequest* get_get_request(Block* block) {
    auto request{new GetRequest};

    request->set_allocated_block(block);

    return request;
}

GetRequest* get_get_request(File* file) {
    auto request{new GetRequest};

    request->set_allocated_file(file);

    return request;
}

GetResponse* get_get_response(const GetRequest& request) {
    auto response{new GetResponse};

    response->set_allocated_request(new GetRequest(request));
    response->set_data("");

    return response;
}
