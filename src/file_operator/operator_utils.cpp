#include "file_operator/operator_utils.h"
#include "message_utils.h"
#include "type/definitions.h"

#include <algorithm>
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

vector<pair<msg::Data, bool>> get_data_spaces(
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
