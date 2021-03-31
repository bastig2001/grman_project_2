#include "file_operations.h"
#include "sync_utils.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <regex>

using namespace std;
using namespace filesystem;

Block* get_block(const string&);

template<typename T>
unsigned long get_timestamp(
    chrono::time_point<T> time_point
) {
    return 
        chrono::duration_cast<chrono::milliseconds>(
            time_point.time_since_epoch()
        ).count();
}


vector<path> get_files(bool include_hidden) {
    vector<path> paths{};

    recursive_directory_iterator file_iterator{current_path()};
    for (auto file: file_iterator) {
        auto path{relative(file)};

        if (file.is_regular_file() 
            && 
            (include_hidden || is_not_hidden(path))
        ) {
            paths.push_back(path);
        }
        else if (file.is_directory() 
                && 
                !include_hidden && is_hidden(path)
        ) {
            file_iterator.disable_recursion_pending();
        }
    }

    return paths;
}

bool is_hidden(const path& path) {
    return regex_match(path.c_str(), regex{"^(.*\\/\\.|\\.).+$"});
}

bool is_not_hidden(const path& path) {
    return !is_hidden(path);
}


ShowFiles* get_show_files(QueryOptions* options) {
    auto show_files{new ShowFiles};

    show_files->set_allocated_options(options);

    return show_files;
}


QueryOptions* get_query_options(
    bool include_hidden, 
    optional<chrono::time_point<chrono::system_clock>> changed_after
) {
    auto query_options{new QueryOptions};

    query_options->set_include_hidden(include_hidden);

    if (changed_after.has_value()) {
        query_options->set_timestamp(get_timestamp(changed_after.value()));
    }

    return query_options;
}


SyncRequest* get_sync_request(Block* block) {
    auto request{new SyncRequest};

    request->set_allocated_starting_block(block);
    
    ifstream file_stream{block->file_name(), ios::binary};
    auto signatures{
        get_weak_signatures(
            file_stream, 
            file_size(block->file_name()), 
            block->size(), 
            block->offset()
        )};
    for (auto signature: signatures) {
        request->add_weak_signatures(signature);
    }

    return request;
}


CheckFileRequest* get_check_file_request(File* file) {
    auto request{new CheckFileRequest};

    request->set_allocated_file(file);

    return request;
}

CheckFileResponse* get_check_file_response(const CheckFileRequest& request) {
    auto response{new CheckFileResponse};
    auto file{new File(request.file())};

    response->set_allocated_requested_file(file);

    if (exists(file->file_name())) {
        auto local_file{get_file(file->file_name())};

        if (local_file->timestamp() == file->timestamp() 
            && 
            local_file->size() == file->size()
            &&
            local_file->signature() == file->signature()
        ) {
            response->set_match(true);
        }
        else if (local_file->timestamp() > file->timestamp()) {
            response->set_request_syncing(true);
        }
        else {
            response->set_allocated_sync_request(
                get_sync_request(get_block(file->file_name()))
            );
        }

        delete local_file;
    }
    else {
        response->set_requesting_file(true);
    }

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


File* get_file(const path& path) {
    auto file{new File};

    file->set_file_name(path);
    file->set_timestamp(get_timestamp(last_write_time(path)));
    file->set_size(file_size(path));

    ifstream file_stream{path, ios::binary};
    file->set_signature(get_strong_signature(file_stream));

    return file;
}

Block* get_block(const string& file_name) {
    auto block{new Block};

    block->set_file_name(file_name);
    block->set_offset(0);
    block->set_size(6000);

    return block;
}


#ifdef UNIT_TESTS
#include "unit_tests/doctest_utils.h"
#include <doctest.h>

TEST_SUITE("file operations utils") {
    TEST_CASE("hidden path") {
        vector<path> hidden_paths
            {".gitignore", ".git/", "/.ignore.txt", "build/.gitkeep", ".hidden/.hidden"};
        path hidden_path{};
        DOCTEST_VALUE_PARAMETERIZED_DATA(hidden_path, hidden_paths);

        CHECK(is_hidden(hidden_path));
        CHECK(!is_not_hidden(hidden_path));
    }
    TEST_CASE("not hidden path") {
        vector<path> not_hidden_paths
            {"LICENSE", "build", "/log.txt", "src/main.cpp", "include/unit_tests"};
        path not_hidden_path{};
        DOCTEST_VALUE_PARAMETERIZED_DATA(not_hidden_path, not_hidden_paths);

        CHECK(is_not_hidden(not_hidden_path));
        CHECK(!is_hidden(not_hidden_path));
    }
}

#endif
