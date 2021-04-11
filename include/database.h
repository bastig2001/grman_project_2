#pragma once

#include "messages/basic.h"
#include "type/result.h"
#include "type/definitions.h"

#include <optional>
#include <vector>


namespace db {
    void create(bool exists);

    void insert_or_replace_file(msg::File);
    void insert_or_replace_files(std::vector<msg::File>);
    void delete_file(FileName);
    Result<msg::File> get_file(FileName);
    std::vector<msg::File> get_files();

    void insert_or_replace_removed(msg::Removed);
    void insert_or_replace_removed(std::vector<msg::Removed>);
    void delete_removed(FileName);
    Result<msg::Removed> get_removed(FileName);

    void insert_or_replace_last_checked(Timestamp);
    std::optional<Timestamp> get_last_checked();

    void insert_or_replace_data(std::vector<msg::Data>);
    std::vector<msg::Data> get_and_remove_data(FileName); 
}
