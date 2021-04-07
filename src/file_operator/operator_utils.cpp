#include "file_operator/operator_utils.h"
#include "file_operator/message_utils.h"
#include "file_operator/signatures.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace std;


vector<Block*> get_blocks_between(
    vector<Offset>&& offsets,
    size_t file_size,
    const string& file_name,
    BlockSize block_size
) {
    auto block_values{get_blocks_between(move(offsets), file_size, block_size)};

    vector<Block*> blocks{};
    blocks.reserve(block_values.size());

    for (auto [offset, size]: block_values) {
        blocks.push_back(block(file_name, offset, size));
    }

    return blocks;
}

vector<pair<Offset, BlockSize>> get_blocks_between(
    vector<Offset>&& offsets,
    size_t file_size,
    BlockSize block_size
) {
    sort(offsets.begin(), offsets.end());

    vector<pair<Offset, BlockSize>> blocks{};

    Offset last_block_end{0};
    for (Offset offset: offsets) {
        if (last_block_end < offset) {
            blocks.push_back({last_block_end, offset - last_block_end});
        }

        last_block_end = offset + block_size;
    }

    if (last_block_end < file_size) {
        blocks.push_back(
            {last_block_end, file_size - last_block_end}
        );
    }

    return blocks;
}
