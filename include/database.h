#pragma once

#include "messages/basic.h"
#include "type/result.h"
#include "type/definitions.h"

#include <optional>
#include <vector>


// All functions to interact with the database(s)
namespace db {
    void create(bool exists);

    void insert_file(msg::File);
    void insert_files(std::vector<msg::File>);
    void update_file(msg::File);
    void delete_file(FileName);
    Result<msg::File> get_file(FileName);
    std::vector<msg::File> get_files();

    void insert_removed(msg::Removed);
    void insert_removed(std::vector<msg::Removed>);
    void delete_removed(FileName);
    Result<msg::Removed> get_removed(FileName);

    void insert_or_update_last_checked(Timestamp);
    std::optional<Timestamp> get_last_checked();

    void insert_data(std::vector<msg::Data>);
    std::vector<msg::Data> get_and_remove_data(FileName); 
}
