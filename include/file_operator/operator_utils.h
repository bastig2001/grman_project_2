#pragma once

#include "config.h"
#include "file_operator/signatures.h"
#include "messages/basic.h"
#include "messages/all.pb.h"

#include <filesystem>
#include <string>
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

// gets the paths of all files which are to be synced according to the provided config
std::vector<std::filesystem::path> get_file_paths(const Config&);

// gets tha meta information of all files at the given paths 
// and returns all successful reads
std::vector<msg::File> get_files(std::vector<std::filesystem::path>&&);

// correct the file with the given file name based on the data which is in the database
void correct(const FileName&);

// remove the file with the given file name from the filesystem and the database
// and register its removal in the database
void remove(const FileName&);
