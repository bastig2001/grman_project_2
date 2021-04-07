#include "file_operator/operator_utils.h"
#include "file_operator/message_utils.h"

#include <algorithm>
#include <vector>

using namespace std;


vector<Block*> get_blocks_between(
    vector<unsigned long>&& offsets,
    size_t file_size,
    const string& file_name,
    unsigned int block_size
) {
    sort(offsets.begin(), offsets.end());

    vector<Block*> blocks{};

    unsigned long last_block_end{0};
    for (unsigned long offset: offsets) {
        if (last_block_end < offset) {
            blocks.push_back(
                block(file_name, last_block_end, offset - last_block_end)
            );
        }

        last_block_end = offset + block_size;
    }

    if (last_block_end < file_size) {
        blocks.push_back(
            block(file_name, last_block_end, file_size - last_block_end)
        );
    }

    return blocks;
}
