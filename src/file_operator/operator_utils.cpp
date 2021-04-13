#include "file_operator/operator_utils.h"
#include "file_operator/filesystem.h"
#include "file_operator/sync_utils.h"
#include "database.h"
#include "messages/basic.h"
#include "presentation/logger.h"
#include "presentation/format_utils.h"
#include "type/definitions.h"
#include "type/result.h"
#include "type/sequence.h"
#include "messages/sync.pb.h"

#include <filesystem>
#include <regex>
#include <utility>
#include <vector>

using namespace std;


vector<filesystem::path> get_file_paths(const Config& config) {
    return
        Sequence(fs::get_file_paths(config.sync.sync_hidden_files))
        .where([&](const filesystem::path& path){
            return (
                config.logger.file == ""
                ||
                absolute(path) != absolute(filesystem::path(config.logger.file))
            ) &&
                !regex_match(path.c_str(), regex{"^\\.sync.*$"}); // if in .sync folder
        })
        .to_vector();
}

vector<msg::File> get_files(vector<filesystem::path>&& paths) {
    return 
        Sequence(fs::get_files(move(paths)))
        .peek([](Result<msg::File> file){
            file.peek(
                [](auto){},
                [](Error err){
                    logger->error(err.msg);
                }
            );
        })
        .where([](const Result<msg::File>& file){
            return file.is_ok();
        })
        .map<msg::File>([](Result<msg::File> file){
            return file.get_ok();
        })
        .to_vector();
}

void correct(const FileName& file) {
    logger->info("Correcting " + colored(file));

    db::get_file(file)
    .flat_map<bool>([](msg::File file){
        return
            fs::build_file(
                get_data_spaces(
                    db::get_and_remove_data(file.name),
                    file.name,
                    file.size
                ),
                file.name
            );
    })
    .apply(
        [](auto){},
        [&](Error err){ logger->error( err.msg ); }
    );
}

void remove(const FileName& file) {
    logger->info("Removing " + colored(file));

    db::get_file(file)
    .apply(
        [](msg::File file){
            db::insert_removed(file);
            db::delete_file(file.name);
            fs::remove_file(file.name);
        }
    );
}

Corrections* get_corrections(
    vector<BlockPair*>&& pairs,
    const FileName& file,
    bool final
) {
    return ::corrections(
        Sequence(move(pairs))
        .map<Result<Correction*>>([](BlockPair* pair){
            return
            fs::read(
                pair->file_name(), 
                pair->offset_server(), 
                pair->size_server()
            )
            .peek(
                [](auto){},
                [&](Error err){ logger->error(err.msg); }
            )
            .map<Correction*>([&](string data){
                auto block{::block(
                    pair->file_name(),
                    pair->offset_client(),
                    pair->size_client()
                )};
                delete pair;

                return ::correction(
                    block,
                    move(data)
                );
            });
        })
        .where([](Result<Correction*> result){
            return result.is_ok();
        })
        .map<Correction*>([](Result<Correction*> result){ 
            return result.get_ok(); 
        })
        .to_vector(),
        file,
        final
    );
}

