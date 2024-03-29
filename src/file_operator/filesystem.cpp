#include "file_operator/filesystem.h"
#include "file_operator/signatures.h"
#include "messages/basic.h"
#include "type/error.h"
#include "type/result.h"
#include "utils.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <mutex>
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


vector<Result<msg::File>> fs::get_files(const vector<path>& paths) {
    vector<Result<msg::File>> files(paths.size());
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

Result<msg::File> fs::get_file(const path& path) {
    try {
        ifstream file_stream{path, ios::binary};

        return Result<msg::File>::ok(
            msg::File {
                path, 
                get_timestamp(last_write_time(path)), 
                file_size(path),
                ::get_strong_signature(file_stream)
            });
    }
    catch (const exception& err) {
        return Result<msg::File>::err(
            Error{path.string() + ": " + err.what()}
        );
    }
}


Result<vector<WeakSign>> fs::get_request_signatures(const path& file) {
    try {
        ifstream file_stream{file, ios::binary};
        auto size{file_size(file)};
        vector<WeakSign> signatures{};

        for (Offset offset{0}; offset < size; offset += BLOCK_SIZE) {
            signatures.push_back(
                ::get_weak_signature(
                    file_stream,
                    min((unsigned long)BLOCK_SIZE, size - offset),
                    offset
            ));
        }

        return Result<vector<WeakSign>>::ok(move(signatures));
    }
    catch (const exception& err) {
        return Result<vector<WeakSign>>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}

Result<vector<WeakSign>> fs::get_weak_signatures(const path& file) {
    try {
        ifstream file_stream{file, ios::binary};
        auto size{file_size(file)};

        return Result<vector<WeakSign>>::ok(
            ::get_weak_signatures(
                file_stream, 
                size, 
                min(size, (unsigned long)BLOCK_SIZE)
            ));
    }
    catch (const exception& err) {
        return Result<vector<WeakSign>>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}

Result<WeakSign> fs::get_weak_signature(
    const std::filesystem::path& file,
    BlockSize block_size,
    Offset offset
) {
    try {
        ifstream file_stream{file, ios::binary};
        return Result<WeakSign>::ok(
            ::get_weak_signature(file_stream, block_size, offset)
        );
    }
    catch (const exception& err) {
        return Result<WeakSign>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}


Result<StrongSign> fs::get_strong_signature(
    const std::filesystem::path& file,
    BlockSize size,
    Offset offset
) {
    try {
        ifstream file_stream{file, ios::binary};
        vector<char> block(size);

        file_stream.seekg(offset, ios::beg);
        file_stream.read(block.data(), size);


        return Result<StrongSign>::ok(
            ::get_strong_signature(string{block.begin(), block.end()})
        );
    }
    catch (const exception& err) {
        return Result<StrongSign>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}


Result<vector<string>> fs::read(
    const path& file,
    const vector<pair<Offset, BlockSize>>& blocks
) {
    try {
        ifstream file_stream{file, ios::binary};

        vector<string> data{};
        data.reserve(blocks.size());

        for (auto [offset, size]: blocks) {
            vector<char> block(size);

            file_stream.seekg(offset, ios::beg);
            file_stream.read(block.data(), size);

            data.push_back(string{block.begin(), block.end()});
        }

        return Result<vector<string>>::ok(move(data));
    }
    catch (const exception& err) {
        return Result<vector<string>>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}

Result<string> fs::read(
    const path& file,
    Offset offset,
    BlockSize size
) {
    try {
        ifstream file_stream{file, ios::binary};
        vector<char> block(size);

        file_stream.seekg(offset, ios::beg);
        file_stream.read(block.data(), size);

        return Result<string>::ok(string{block.begin(), block.end()});
    }
    catch (const exception& err) {
        return Result<string>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}

Result<string> fs::read(const path& file) {
    try {
        ifstream file_stream{file, ios::binary};
        auto size{file_size(file)};
        vector<char> data(size);

        file_stream.seekg(0, ios::beg);
        file_stream.read(data.data(), size);

        return Result<string>::ok(string{data.begin(), data.end()});
    }
    catch (const exception& err) {
        return Result<string>::err(
            Error{file.string() + ": " + err.what()}
        );
    }
}


Result<bool> fs::write(const path& file, string&& data) {
    try {
        if (file.has_parent_path() && !exists(file.parent_path())) {
            create_directories(file.parent_path());
        }

        ofstream file_stream{file, ios::binary};

        file_stream.write(data.c_str(), data.size());

        return Result<bool>::ok(true);
    }
    catch (const exception& err) {
        return Result<bool>::err(
            Error{"Writing " + file.string() + ": " + err.what()}
        );
    }
}


Result<bool> fs::move_file(const path& old_path, const path& new_path) {
    try {
        if (new_path.has_parent_path() && !exists(new_path.parent_path())) {
            create_directories(new_path.parent_path());
        }
        
        rename(old_path, new_path);
        remove_empty_dir(old_path.parent_path());

        return Result<bool>::ok(true);
    }
    catch (const exception& err) {
        return Result<bool>::err(
            Error{
                "Moving " + old_path.string() + " to " 
                + new_path.string() + ": " + err.what()
            }
        );
    }  
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


// only one file at a time should be build, otherwise there might be a name collision
mutex build_mtx{}; 

Result<bool> fs::build_file(
    vector<pair<msg::Data, bool /* has data */>>&& data,
    const path& path
) {
    lock_guard build_lck{build_mtx};

    try {
        ::path temp_path{::path{".sync"} /= ::path{"tmp"} / path.filename()};

        ofstream file{temp_path, ios::binary};

        for (auto [block, has_data]: data) {
            if (has_data) {
                file.write(block.data.c_str(), block.data.size());
            }
            else {
                file.write(
                    read(path, block.offset, block.size).get_ok().c_str(), 
                    block.size
                );
            }
        }

        return move_file(temp_path, path);
    }
    catch (const exception& err) {
        return Result<bool>::err(
            Error{"Building " + path.string() + ": " + err.what()}
        );
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
