#include "file_operations.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <filesystem>
#include <iterator>
#include <regex>

using namespace std;
using namespace filesystem;

bool is_hidden(const path&);
bool is_not_hidden(const path&);


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
