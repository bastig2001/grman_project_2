#pragma once

#include "config.h"
#include "messages/basic.h"
#include "type/definitions.h"
#include "messages/sync.pb.h"

#include <filesystem>
#include <vector>


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

// returns the corrections (final or not) for the given blocks in the specified file
Corrections* get_corrections(
    std::vector<BlockPair*>&&,
    const FileName&,
    bool final = false
);
