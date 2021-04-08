#include "file_operator/filesystem.h"
#include "file_operator/signatures.h"
#include "utils.h"
#include "messages/all.pb.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <regex>
#include <utility>
#include <vector>

using namespace std;
using namespace filesystem;

void remove_empty_dir(const path&);


vector<path> fs::get_file_paths(bool include_hidden) {
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

bool fs::is_hidden(const path& path) {
    return regex_match(path.c_str(), regex{"^(.*\\/\\.|\\.).+$"});
}

bool fs::is_not_hidden(const path& path) {
    return !is_hidden(path);
}


vector<File*> fs::get_files(const vector<path>& paths) {
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

File* fs::get_file(const path& path) {
    auto file{new File};

    file->set_name(path);
    file->set_timestamp(get_timestamp(last_write_time(path)));
    file->set_size(file_size(path));

    ifstream file_stream{path, ios::binary};
    file->set_signature(::get_strong_signature(file_stream));

    return file;
}


vector<unsigned int> fs::get_request_signatures(const path& file) {
    ifstream file_stream{file, ios::binary};
    auto size{file_size(file)};
    vector<unsigned int> signatures{};

    for (unsigned long offset{0}; offset < size; offset += BLOCK_SIZE) {
        signatures.push_back(
            ::get_weak_signature(
                file_stream,
                min((unsigned long)BLOCK_SIZE, size - offset),
                offset
        ));
    }

    return signatures;
}

vector<unsigned int> fs::get_weak_signatures(const path& file) {
    ifstream file_stream{file, ios::binary};
    auto size{file_size(file)};

    return 
        ::get_weak_signatures(
            file_stream, 
            size, 
            min(size, (unsigned long)BLOCK_SIZE)
        );
}

unsigned int fs::get_weak_signature(
    const std::filesystem::path& file,
    BlockSize block_size,
    Offset offset
) {
    ifstream file_stream{file, ios::binary};
    return ::get_weak_signature(file_stream, block_size, offset);
}


string fs::get_strong_signature(
    const std::filesystem::path& file,
    BlockSize size,
    Offset offset
) {
    ifstream file_stream{file, ios::binary};
    vector<char> block(size);

    file_stream.seekg(offset, ios::beg);
    file_stream.read(block.data(), size);


    return ::get_strong_signature(string{block.begin(), block.end()});
}


vector<string> fs::read(
    const path& file,
    const vector<pair<Offset, BlockSize>>& blocks
) {
    ifstream file_stream{file, ios::binary};

    vector<string> data{};
    data.reserve(blocks.size());

    for (auto [offset, size]: blocks) {
        vector<char> block(size);

        file_stream.seekg(offset, ios::beg);
        file_stream.read(block.data(), size);

        data.push_back(string{block.begin(), block.end()});
    }

    return data;
}

string fs::read(
    const path& file,
    Offset offset,
    BlockSize size
) {
    ifstream file_stream{file, ios::binary};
    vector<char> block(size);

    file_stream.seekg(offset, ios::beg);
    file_stream.read(block.data(), size);

    return string{block.begin(), block.end()};
}

string fs::read(const path& file) {
    ifstream file_stream{file, ios::binary};
    auto size{file_size(file)};
    vector<char> data(size);

    file_stream.seekg(0, ios::beg);
    file_stream.read(data.data(), size);

    return string{data.begin(), data.end()};
}


void fs::move_file(const path& old_path, const path& new_path) {
    if (!exists(new_path.parent_path())) {
        create_directories(new_path.parent_path());
    }
    
    rename(old_path, new_path);
    remove_empty_dir(old_path.parent_path());
}

void fs::remove_file(const path& path) {
    remove(path);
    remove_empty_dir(path.parent_path());
}

void remove_empty_dir(const path& directory) {
    if (filesystem::is_empty(directory)) {
        remove(directory);
    }
}


#ifdef UNIT_TESTS
#include "unit_tests/doctest_utils.h"
#include <doctest.h>

using namespace fs;

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
