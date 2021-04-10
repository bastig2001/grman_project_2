#pragma once

#include "config.h"
#include "message_utils.h"
#include "file_operator/signatures.h"
#include "messages/all.pb.h"

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>


class SyncSystem {
  private:
    std::optional<std::chrono::time_point<std::chrono::system_clock>> 
        last_checked{std::nullopt};
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

    void correct(const File&, const Corrections&);

  public:
    SyncSystem(const SyncConfig&);

    ~SyncSystem();

    Message get_show_files();

    Message get_file_list(const ShowFiles&);

    std::vector<Message> get_sync_requests(const FileList&);

    Message get_sync_response(const SyncRequest&); 

    std::vector<Message> handle_sync_response(const SyncResponse&);

    Message get_sync_response(const SignatureAddendum&);
};
