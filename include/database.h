#pragma once

#include "messages/basic.h"
#include "type/result.h"

#include <vector>


namespace db {
    void insert_file(msg::File);
    void insert_files(std::vector<msg::File>&&);
    void update_file(msg::File);
    void update_files(std::vector<msg::File>&&);
    void delete_file(FileName);
    void delete_files(unsigned int owner);
    Result<msg::File> get_file(FileName);
    std::vector<msg::File> get_files();

    void insert_or_replace_removed(msg::File);
    void insert_or_replace_removed(std::vector<msg::File>&&);
    void delete_removed(FileName);
    Result<msg::File> get_removed(FileName);

    void insert_or_update_block(msg::Block);
    void insert_or_update_blocks(std::vector<msg::Block>&&);
    void delete_blocks(FileName);
    void delete_blocks(FileName, unsigned int owner);
    void delete_blocks(unsigned int owner);
    std::vector<msg::Block> get_blocks(FileName, unsigned int owner);
}
