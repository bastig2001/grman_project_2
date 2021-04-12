#include "database.h"
#include "presentation/logger.h"
#include "messages/basic.h"
#include "type/definitions.h"

#include <mutex>
#include <sqlite_orm/sqlite_orm.h>
#include <optional>
#include <vector>

using namespace std;
using namespace sqlite_orm;


struct LastChecked {
    int id;
    Timestamp timestamp;
};


mutex permanent_db_mtx{};
mutex in_memory_db_mtx{};

auto permanent_db{make_storage(
    ".sync/db.sqlite",
    make_table(
        "file",
        make_column("name",      &msg::File::name, primary_key()),
        make_column("timestamp", &msg::File::timestamp),
        make_column("size",      &msg::File::size),
        make_column("signature", &msg::File::signature)
    ),
    make_table(
        "removed",
        make_column("name",      &msg::Removed::name, primary_key()),
        make_column("timestamp", &msg::Removed::timestamp)
    ),
    make_table(
        "last_checked",
        make_column("id",        &LastChecked::id, primary_key()),
        make_column("timestamp", &LastChecked::timestamp)
    )
)};

auto in_memory_db{make_storage(
    "",
    make_table(
        "data",
        make_column("file_name", &msg::Data::file_name),
        make_column("offset",    &msg::Data::offset),
        make_column("size",      &msg::Data::size),
        make_column("data",      &msg::Data::data)
    )
)};


void db::create(bool exists) {
    scoped_lock db_lck{permanent_db_mtx, in_memory_db_mtx};

    if (!exists) {
        permanent_db.sync_schema();
    }

    permanent_db.remove_all<msg::File>();
    
    in_memory_db.sync_schema();
}


void db::insert_file(msg::File file) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.replace(move(file));
}

void db::insert_files(vector<msg::File> files) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.replace_range(files.begin(), files.end());
}

void db::update_file(msg::File file) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.update(file);
}

void db::delete_file(FileName name) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.remove<msg::File>(name);
}

Result<msg::File> db::get_file(FileName name) {
    lock_guard db_lck{permanent_db_mtx};

    if (auto file{permanent_db.get_optional<msg::File>(name)}) {
        return Result<msg::File>::ok(move(file.value()));
    }
    else {
        return Result<msg::File>::err(
            Error{name + " not found in 'file' table!"}
        );
    }
}

vector<msg::File> db::get_files() {
    lock_guard db_lck{permanent_db_mtx};

    return permanent_db.get_all<msg::File>();
}


void db::insert_removed(msg::Removed file) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.replace(move(file));
}

void db::insert_removed(vector<msg::Removed> files) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.replace_range(files.begin(), files.end());
}

void db::delete_removed(FileName name) {
    lock_guard db_lck{permanent_db_mtx};

    permanent_db.remove<msg::Removed>(name);
}

Result<msg::Removed> db::get_removed(FileName name) {
    lock_guard db_lck{permanent_db_mtx};

    if (auto file{permanent_db.get_optional<msg::Removed>(name)}) {
        return Result<msg::Removed>::ok(move(file.value()));
    }
    else {
        return Result<msg::Removed>::err(
            Error{name + " not found in 'removed' table!"}
        );
    }
}


void db::insert_or_update_last_checked(Timestamp timestamp) {
    lock_guard db_lck{permanent_db_mtx};

    if (auto last_checked{permanent_db.get_optional<LastChecked>(0)}) {
        permanent_db.update(LastChecked{0, timestamp});
    }
    else {
        permanent_db.replace(LastChecked{0, timestamp});
    }
}

optional<Timestamp> db::get_last_checked() {
    lock_guard db_lck{permanent_db_mtx};

    if (auto last_checked{permanent_db.get_optional<LastChecked>(0)}) {
        return last_checked.value().timestamp;
    }
    else {
        return nullopt;
    }
}


void db::insert_data(vector<msg::Data> data) {
    lock_guard db_lck{in_memory_db_mtx};

    in_memory_db.replace_range(data.begin(), data.end());
}

vector<msg::Data> db::get_and_remove_data(FileName file) {
    lock_guard db_lck{in_memory_db_mtx};

    return in_memory_db.get_all<msg::Data>(
        where(c(&msg::Data::file_name) == file),
        order_by(&msg::Data::offset)
    );
}
