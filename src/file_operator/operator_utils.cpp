#include "file_operator/operator_utils.h"
#include "file_operator/filesystem.h"
#include "database.h"
#include "message_utils.h"
#include "messages/basic.h"
#include "presentation/logger.h"
#include "presentation/format_utils.h"
#include "type/definitions.h"
#include "type/result.h"
#include "type/sequence.h"

#include <algorithm>
#include <filesystem>
#include <regex>
#include <utility>
#include <vector>

using namespace std;


vector<BlockPair*> get_block_pairs_between(
    vector<BlockPair*>& matching,
    const FileName& name,
    size_t client_file_size,
    size_t server_file_size
) {
    sort(
        matching.begin(),
        matching.end(),
        [](auto a, auto b){
            return a->offset_client() < b->offset_client();
        }
    );

    vector<BlockPair*> non_matching{};

    Offset last_client_block_end{0};
    Offset last_server_block_end{0};
    for (auto pair: matching) {
        if (last_client_block_end < pair->offset_client()
            ||
            last_server_block_end < pair->offset_server()
        ) {
            non_matching.push_back(block_pair(
                name,
                last_client_block_end,
                last_server_block_end,
                pair->offset_client() - last_client_block_end,
                pair->offset_server() - last_server_block_end
            ));
        }

        last_client_block_end = pair->offset_client() + pair->size_client();
        last_server_block_end = pair->offset_server() + pair->size_server();
    }

    if (last_client_block_end < client_file_size
        ||
        last_server_block_end < server_file_size
    ) {
        non_matching.push_back(block_pair(
            name,
            last_client_block_end,
            last_server_block_end,
            client_file_size - last_client_block_end,
            server_file_size - last_server_block_end
        ));
    }

    return non_matching;
}

vector<pair<msg::Data, bool /* has data */>> get_data_spaces(
    vector<msg::Data>&& data,
    const FileName& file_name,
    size_t file_size
) {
    sort(
        data.begin(),
        data.end(),
        [](auto a, auto b){
            return a.offset < b.offset;
        }
    );

    vector<pair<msg::Data, bool>> blocks{};

    Offset last_block_end{0};
    for (auto block: data) {
        if (last_block_end < block.offset) {
            blocks.push_back({
                msg::Data(
                    file_name, 
                    last_block_end, 
                    block.offset - last_block_end, 
                    ""
                ),
                false
            });
        }
        
        last_block_end = block.offset + block.size;

        blocks.push_back({move(block), true});        
    }

    if (last_block_end < file_size) {        
        blocks.push_back({
            msg::Data(
                file_name, 
                last_block_end, 
                file_size - last_block_end, 
                ""
            ),
            false
        });
    }

    return blocks;
}

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
