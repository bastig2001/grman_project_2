#include "file_operator.h"
#include "config.h"
#include "exit_code.h"
#include "file_operations.h"
#include "internal_msg.h"
#include "pipe.h"
#include "unit_tests.p/messages/all.pb.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <unordered_map>

using namespace std;

struct FileData {
    optional<chrono::time_point<chrono::system_clock>> last_checked{nullopt};
    unordered_map<string, File*> files_by_names{};
    unordered_map<string, File*> files_by_signatures{};
};

void create_file_index(const SyncConfig&, FileData&);
InternalMsg get_response(
    const InternalMsg&, 
    const SyncConfig&, 
    FileData&, 
    SendingPipe<InternalMsg>*
);
Message handle_message(const Message&, const SyncConfig&, FileData&);


int run_file_operator(
    const SyncConfig& config,
    Pipe<InternalMsg>& inbox
) {
    ExitCode exit_code;
    FileData data{};

    try {
        create_file_index(config, data);

        InternalMsg msg;
        while (inbox >> msg) {
            *msg.originator << get_response(msg, config, data, &inbox);
        }

        exit_code = Success;
    }
    catch (exception& err) {
        logger->critical(
            "Following exception occurred during file operator execution: " 
            + string{err.what()}
        );

        exit_code = FileOperatorException;
    }

    inbox.close();

    return exit_code;
}

void create_file_index(const SyncConfig& config, FileData& data) {
    auto files = get_files(config.sync_hidden_files);
    logger->debug("Files to sync:\n" + vector_to_string(files, "\n"));

    for (const auto& path: files) {
        auto file{get_file(path)};
        data.files_by_names.insert({file->file_name(), file});
        data.files_by_signatures.insert({file->signature(), file});
    }
}

InternalMsg get_response(
    const InternalMsg& request, 
    const SyncConfig& config, 
    FileData& data,
    SendingPipe<InternalMsg>* inbox
) {
    Message msg{};

    switch (request.type) {
        case InternalMsgType::ServerWaits:
            return InternalMsg(InternalMsgType::FileOperatorStarted, inbox);
        case InternalMsgType::ClientWaits:
            *request.originator 
                << InternalMsg(InternalMsgType::FileOperatorStarted, inbox);
            msg.set_allocated_show_files(
                get_show_files(
                    get_query_options(
                        config.sync_hidden_files, 
                        data.last_checked
            )));

            data.last_checked = chrono::system_clock::now();

            return InternalMsg::to_originator(inbox, msg);
        case InternalMsgType::HandleMessage:
            return 
                InternalMsg::to_originator(
                    inbox, 
                    handle_message(request.msg, config, data)
                );
        default:
            // There shouldn't be anything else
            throw invalid_argument(
                "Received invalid InternalMsgType " + 
                to_string((int)request.type)
            );
    }
}

Message handle_message(
    const Message& request, 
    const SyncConfig&, 
    FileData& data
) {
    Message response{};

    switch (request.message_case()) {
        case Message::kShowFiles:
            response.set_allocated_file_list(
                get_file_list(
                    request.show_files(), 
                    to_vector(data.files_by_names)
            ));
            break;
        case Message::kFileList:
            break;
        case Message::kSyncRequest:
            break;
        case Message::kSyncResponse:
            break;
        case Message::kSignatureAddendum:
            break;
        case Message::kCheckFileRequest:
            break;
        case Message::kCheckFileResponse:
            break;
        case Message::kFileRequest:
            break;
        case Message::kFileResponse:
            break;
        default: 
            break;
    }

    return response;
}

