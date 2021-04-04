#include "file_operator.h"
#include "config.h"
#include "exit_code.h"
#include "file_operations.h"
#include "internal_msg.h"
#include "message_utils.h"
#include "pipe.h"
#include "messages/all.pb.h"
#include "presentation/logger.h"

#include <fmt/format.h>
#include <fmt/color.h>
#include <algorithm>
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
    const InternalMsgWithOriginator&, 
    const SyncConfig&, 
    FileData&
);
vector<InternalMsg> wrap_messages(const vector<Message>&);
vector<Message> handle_msg(const Message&, const SyncConfig&, FileData&);

// colouring the file name
string colored(const File& file) {
    return fmt::format(fg(fmt::color::burly_wood), file.file_name());
}


int run_file_operator(
    const SyncConfig& config,
    ReceivingPipe<InternalMsgWithOriginator>& inbox
) {
    ExitCode exit_code;
    FileData data{};

    try {
        create_file_index(config, data);

        while (auto optional_msg{inbox.receive()}) {
            auto msg{optional_msg.value()};
            msg.originator.send(get_response(msg, config, data));
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
    auto file_paths = get_file_paths(config.sync_hidden_files);
    logger->debug("Files to sync:\n" + vector_to_string(file_paths, "\n"));

    for (auto file: get_files(file_paths)) {
        data.files_by_names.insert({file->file_name(), file});
        data.files_by_signatures.insert({file->signature(), file});
    }
}

vector<InternalMsg> get_response(
    const InternalMsgWithOriginator& request, 
    const SyncConfig& config, 
    FileData& data
) {
    Message msg{};

    switch (request.type) {
        case InternalMsgType::ServerWaits:
            return {InternalMsg(InternalMsgType::FileOperatorStarted)};
        case InternalMsgType::ClientWaits:
            msg.set_allocated_show_files(
                get_show_files(
                    get_query_options(
                        config.sync_hidden_files, 
                        data.last_checked
            )));

            data.last_checked = chrono::system_clock::now();

            return {
                InternalMsg(InternalMsgType::FileOperatorStarted),
                get_msg_to_originator(msg)
            };
        case InternalMsgType::HandleMessage:
            return wrap_messages(handle_msg(request.msg, config, data));
        default:
            // There shouldn't be anything else
            throw invalid_argument(
                "Received invalid InternalMsgType " + 
                to_string((int)request.type)
            );
    }
}

vector<InternalMsg> wrap_messages(
    const vector<Message>& msgs
) {
    vector<InternalMsg> internal_msgs{};
    internal_msgs.reserve(msgs.size());

    for (auto msg: msgs) {
        internal_msgs.push_back(get_msg_to_originator(msg));
    }

    return internal_msgs;
}


Message get_file_list_msg(const ShowFiles&, const SyncConfig&, FileData&);
vector<Message> get_sync_requests(const FileList&, FileData&);


vector<Message> handle_msg(
    const Message& request, 
    const SyncConfig& config, 
    FileData& data
) {
    switch (request.message_case()) {
        case Message::kShowFiles:
            return {get_file_list_msg(request.show_files(), config, data)};
        case Message::kFileList:
            return get_sync_requests(request.file_list(), data);
        case Message::kSyncRequest:
            return {};
        case Message::kSyncResponse:
            return {};
        case Message::kSignatureAddendum:
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
    const ShowFiles& request, 
    const SyncConfig& config, 
    FileData& data
) {
    Message response{};

    response.set_allocated_file_list(
        get_file_list(
            get_query_options(
                config.sync_hidden_files && request.options().include_hidden(),
                request.options().has_timestamp()
                ? optional{request.options().timestamp()}
                : nullopt), 
            to_vector(data.files_by_names)
    ));

    return response;
}

vector<Message> get_sync_requests(
    const FileList& server_list,
    FileData& data
) {
    vector<Message> msgs{};
    vector<string> checked_file_names{};

    for (auto& file: server_list.files()) {
        Message msg{};

        if (contains(data.files_by_names, file.file_name())) {
            checked_file_names.push_back(file.file_name());

            auto local_file = data.files_by_names[file.file_name()];
            if (!(local_file->timestamp() == file.timestamp() 
                    && 
                  local_file->size() == file.size()
                    &&
                  local_file->signature() == file.signature()
            )) {
                logger->info("Starting syncing process for " + colored(file));

                msg.set_allocated_sync_request(
                    get_sync_request(new File(*local_file))
                );
                msgs.push_back(move(msg));
            } 
            else {
                logger->info(colored(file) + " needs no syncing");
            }
        }
        else if (contains(data.files_by_signatures, file.signature())) {
            auto local_file = data.files_by_signatures[file.signature()];
            checked_file_names.push_back(local_file->file_name());

            if (local_file->timestamp() == file.timestamp() 
                    && 
                local_file->size() == file.size()
            ) {
                logger->info(
                    "Moving " + colored(*local_file) + 
                    " to " + colored(file)
                );

                move_file(local_file->file_name(), file.file_name());
            } 
            else {
                logger->info(
                    "Starting syncing process for " + colored(*local_file)
                );

                msg.set_allocated_sync_request(
                    get_sync_request(new File(*local_file))
                );
                msgs.push_back(move(msg));
            }
        }
        else {
            msg.set_allocated_file_request(get_file_request(new File(file)));
            msgs.push_back(move(msg));
        }
    }

    for (auto [file_name, file]: data.files_by_names) {
        if (!contains(checked_file_names, file_name)) {
            logger->info("Starting syncing process for " + colored(*file));

            Message msg{};
            msg.set_allocated_sync_request(
                get_sync_request(new File(*file))
            );
            msgs.push_back(move(msg));
        }
    }

    return msgs;
}

