#pragma once

#include "config.h"
#include "file_operator/signatures.h"
#include "messages/all.pb.h"

#include <fmt/format.h>
#include <fmt/color.h>
#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>


using FileName = std::string;

class SyncSystem {
  private:
    std::optional<std::chrono::time_point<std::chrono::system_clock>> 
        last_checked{std::nullopt};
    std::unordered_map<FileName, File*> files{}; // existing
    std::unordered_map<FileName, File*> waiting_for_sync{};
    std::unordered_map<FileName, File*> removed{};
    std::unordered_map<
        FileName, 
        std::unordered_map<Offset /* local */, Offset /* client */>
    > matching_offsets{};
    std::unordered_map<FileName, Corrections*> postponed_corrections{};
    const SyncConfig& config;

    Message start_sync(File*);
    Message notify_already_removed(const File&);
    Message request(const File&);

    void remove(File*);

    Message sync(const SyncRequest&);
    Message respond_already_removed(const File&);
    Message respond_requesting(const File&);

    Message get_file(const File&);

    std::vector<Message> sync(const SyncResponse&);

  public:
    SyncSystem(const SyncConfig&);

    ~SyncSystem();

    Message get_show_files();

    Message get_file_list(const ShowFiles&);

    std::vector<Message> get_sync_requests(const FileList&);

    Message get_sync_response(const SyncRequest&); 

    std::vector<Message> handle_sync_response(const SyncResponse&);
};


// colouring the file name
inline std::string colored(const File& file) {
    return fmt::format(fg(fmt::color::burly_wood), file.name());
}
