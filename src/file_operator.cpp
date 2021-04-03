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
vector<InternalMsg> get_response(
    const InternalMsg&, 
    const SyncConfig&, 
    FileData&, 
    SendingPipe<InternalMsg>*
);
vector<InternalMsg> get_to_originator_msgs(
    SendingPipe<InternalMsg>*, 
    const vector<Message>&
);
vector<Message> handle_msg(const Message&, const SyncConfig&, FileData&);
Message get_file_list_msg(const Message&, const SyncConfig&, FileData&);


int run_file_operator(
    const SyncConfig& config,
    Pipe<InternalMsg>& inbox
) {
    ExitCode exit_code;
    FileData data{};

    try {
        create_file_index(config, data);

        while (auto optional_msg{inbox.receive()}) {
            auto msg{optional_msg.value()};
            msg.originator->send(get_response(msg, config, data, &inbox));
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

vector<InternalMsg> get_response(
    const InternalMsg& request, 
    const SyncConfig& config, 
    FileData& data,
    SendingPipe<InternalMsg>* inbox
) {
    Message msg{};

    switch (request.type) {
        case InternalMsgType::ServerWaits:
            return {InternalMsg(InternalMsgType::FileOperatorStarted, inbox)};
        case InternalMsgType::ClientWaits:
            msg.set_allocated_show_files(
                get_show_files(
                    get_query_options(
                        config.sync_hidden_files, 
                        data.last_checked
            )));

            data.last_checked = chrono::system_clock::now();

            return {
                InternalMsg(InternalMsgType::FileOperatorStarted, inbox),
                InternalMsg::to_originator(inbox, msg)
            };
        case InternalMsgType::HandleMessage:
            return 
                get_to_originator_msgs(
                    inbox, 
                    handle_msg(request.msg, config, data)
                );
        default:
            // There shouldn't be anything else
            throw invalid_argument(
                "Received invalid InternalMsgType " + 
                to_string((int)request.type)
            );
    }
}

vector<InternalMsg> get_to_originator_msgs(
    SendingPipe<InternalMsg>* inbox, 
    const vector<Message>& msgs
) {
    vector<InternalMsg> internal_msgs{};
    internal_msgs.reserve(msgs.size());

    for (auto msg: msgs) {
        internal_msgs.push_back(InternalMsg::to_originator(inbox, msg));
    }

    return internal_msgs;
}

vector<Message> handle_msg(
    const Message& request, 
    const SyncConfig& config, 
    FileData& data
) {
    switch (request.message_case()) {
        case Message::kShowFiles:
            return {get_file_list_msg(request, config, data)};
        case Message::kFileList:
            return {};
        case Message::kSyncRequest:
            return {};
        case Message::kSyncResponse:
            return {};
        case Message::kSignatureAddendum:
            return {};
        case Message::kCheckFileRequest:
            return {};
        case Message::kCheckFileResponse:
            return {};
        case Message::kFileRequest:
            return {};
        case Message::kFileResponse:
            return {};
        default: 
            return {};
    }
}

Message get_file_list_msg(
    const Message& request, 
    const SyncConfig& config, 
    FileData& data
) {
    Message response{};

    response.set_allocated_file_list(
        get_file_list(
            get_query_options(
                config.sync_hidden_files 
                    && 
                request.show_files().options().include_hidden(), 
                request.show_files().options().has_timestamp()
                ? optional{request.show_files().options().timestamp()}
                : nullopt), 
            to_vector(data.files_by_names)
    ));

    return response;
}

