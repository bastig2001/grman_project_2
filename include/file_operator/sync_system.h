#pragma once

#include "config.h"
#include "messages/basic.h"
#include "type/definitions.h"
#include "type/result.h"
#include "messages/all.pb.h"

#include <filesystem>
#include <vector>


// The common operations for the synchronisation process
class SyncSystem {
  private:
    const Config& config;

    Result<Message> start_sync(msg::File);
    Message notify_already_removed(const File&);
    Message request(const File&);

    Result<Message> sync(const SyncRequest&, msg::File);
    Message respond_already_removed(const File&);
    Message respond_requesting(const File&);

    std::vector<Message> sync(const SyncResponse&);

  public:
    SyncSystem(const Config&);

    void check_filesystem();

    Message get_show_files();

    Message get_file_list(const ShowFiles&);

    std::vector<Message> get_sync_requests(const FileList&);

    Message get_sync_response(const SyncRequest&); 

    std::vector<Message> handle_sync_response(const SyncResponse&);

    Message get_sync_response(const SignatureAddendum&);

    Message get_file(const File&);

    Message create_file(const FileResponse&);

    Message correct(const Corrections&);
};
