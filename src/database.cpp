#include "database.h"
#include "presentation/logger.h"
#include "messages/basic.h"
#include "type/definitions.h"

#include <sqlite_orm/sqlite_orm.h>
#include <mutex>
#include <vector>

using namespace std;
using namespace sqlite_orm;


mutex permanent_db_mtx{};
mutex in_memory_db_mtx{};

auto permanent_db{make_storage(
    "",
    make_table(
        "file",
        make_column("name",      &msg::File::name, primary_key()),
        make_column("timestamp", &msg::File::timestamp),
        make_column("size",      &msg::File::size),
        make_column("signature", &msg::File::signature),
        make_column("owner",     &msg::File::owner)
    ),
    make_table(
        "removed",
        make_column("name",      &msg::File::name, primary_key()),
        make_column("timestamp", &msg::File::timestamp),
        make_column("size",      &msg::File::size),
        make_column("signature", &msg::File::signature),
        make_column("owner",     &msg::File::owner)
    )
)};

auto in_memory_db{make_storage(
    "",
    make_table(
        "block",
        make_column("id",               &msg::Block::id, autoincrement(), primary_key()),
        make_column("file_name",        &msg::Block::file_name),
        make_column("offset",           &msg::Block::offset),
        make_column("size",             &msg::Block::size),
        make_column("owner",            &msg::Block::owner),
        make_column("weak_signature",   &msg::Block::weak_signature),
        make_column("strong_signature", &msg::Block::strong_signature),
        make_column("data",             &msg::Block::data)
    )
)};


void db::insert_file(msg::File) {

}

void db::insert_files(vector<msg::File>&&) {

}

void db::update_file(msg::File) {

}

void db::update_files(vector<msg::File>&&) {

}

void db::delete_file(FileName) {

}

void db::delete_files(unsigned int owner) {

}

Result<msg::File> db::get_file(FileName) {

}

vector<msg::File> db::get_files() {

}


void db::insert_or_replace_removed(msg::File) {

}

void db::insert_or_replace_removed(vector<msg::File>&&) {

}

void db::delete_removed(FileName) {

}

Result<msg::File> db::get_removed(FileName) {

}


void db::insert_or_update_block(msg::Block) {

}

void db::insert_or_update_blocks(vector<msg::Block>&&) {

}

void db::delete_blocks(FileName) {

}

void db::delete_blocks(FileName, unsigned int owner) {

}

void db::delete_blocks(unsigned int owner) {

}

vector<msg::Block> get_blocks(FileName, unsigned int owner) {

}
