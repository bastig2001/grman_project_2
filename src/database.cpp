#include "database.h"
#include "presentation/logger.h"
#include "messages/basic.h"
#include "type/definitions.h"

#include <sqlite_orm/sqlite_orm.h>
#include <optional>
#include <vector>

using namespace std;
using namespace sqlite_orm;


struct LastChecked {
    int id;
    Timestamp timestamp;
};

auto permanent_db{make_storage(
    "",
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


void db::create() {
    permanent_db.sync_schema();
}


void db::insert_or_replace_file(msg::File file) {
    permanent_db.replace(move(file));
}

void db::insert_or_replace_files(vector<msg::File> files) {
    permanent_db.replace_range(files.begin(), files.end());
}

void db::delete_file(FileName name) {
    permanent_db.remove<msg::File>(name);
}

Result<msg::File> db::get_file(FileName name) {
    if (auto file{permanent_db.get_optional<msg::File>(name)}) {
        return Result<msg::File>::ok(move(file.value()));
    }
    else {
        return Result<msg::File>::err(
            Error{0, name + " not found in 'file' table!"}
        );
    }
}

vector<msg::File> db::get_files() {
    return permanent_db.get_all<msg::File>();
}


void db::insert_or_replace_removed(msg::Removed file) {
    permanent_db.replace(move(file));
}

void db::insert_or_replace_removed(vector<msg::Removed> files) {
    permanent_db.replace_range(files.begin(), files.end());
}

void db::delete_removed(FileName name) {
    permanent_db.remove<msg::Removed>(name);
}

Result<msg::Removed> db::get_removed(FileName name) {
    if (auto file{permanent_db.get_optional<msg::Removed>(name)}) {
        return Result<msg::Removed>::ok(move(file.value()));
    }
    else {
        return Result<msg::Removed>::err(
            Error{0, name + " not found in 'removed' table!"}
        );
    }
}


void db::insert_or_replace_last_checked(Timestamp timestamp) {
    permanent_db.replace(LastChecked{0, timestamp});
}

optional<Timestamp> db::get_last_checked() {
    if (auto last_checked{permanent_db.get_optional<LastChecked>(0)}) {
        return last_checked.value().timestamp;
    }
    else {
        return nullopt;
    }
}
