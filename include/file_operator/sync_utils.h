#pragma once

#include "messages/basic.h"
#include "type/definitions.h"
#include "messages/all.pb.h"

#include <utility>
#include <vector>


// returns the block pairs which fill the mssing spaces in the given block pairs
std::vector<BlockPair*> get_block_pairs_between(
    std::vector<BlockPair*>& matching,
    const FileName&,
    size_t client_file_size,
    size_t server_file_size
);

// returns the data blocks which fill the missing spaces in the given blocks
// and denote if the block has data
std::vector<std::pair<msg::Data, bool /* has data */>> get_data_spaces(
    std::vector<msg::Data>&& data,
    const FileName&,
    size_t file_size
);

