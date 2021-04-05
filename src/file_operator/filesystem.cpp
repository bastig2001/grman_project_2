#include "file_operator/filesystem.h"
#include "file_operator/signatures.h"
#include "utils.h"
#include "messages/info.pb.h"
#include "messages/sync.pb.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <regex>

using namespace std;
using namespace filesystem;

void remove_empty_dir(const path&);


vector<path> get_file_paths(bool include_hidden) {
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


vector<File*> get_files(const vector<path>& paths) {
    vector<File*> files(paths.size());
    transform(
        paths.begin(),
        paths.end(),
        files.begin(),
        [](path file_path){
            return get_file(file_path);
        }
    );

    return files;
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


void move_file(const path& old_path, const path& new_path) {
    if (!exists(new_path.parent_path())) {
        create_directories(new_path.parent_path());
    }
    
    rename(old_path, new_path);
    remove_empty_dir(old_path.parent_path());
}

void remove_file(const path& path) {
    remove(path);
    remove_empty_dir(path.parent_path());
}

void remove_empty_dir(const path& directory) {
    if (filesystem::is_empty(directory)) {
        remove(directory);
    }
}


vector<File*> to_vector(const unordered_map<string, File*>& file_map) {
    vector<File*> files(file_map.size());
    transform(
        file_map.begin(), 
        file_map.end(), 
        files.begin(), 
        [](auto pair){ return pair.second; }
    );

    return files;
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
