#include "file_operator.h"
#include "config.h"
#include "exit_code.h"
#include "internal_msg.h"
#include "pipe.h"
#include "file_operator/message_utils.h"
#include "file_operator/sync_system.h"
#include "messages/all.pb.h"
#include "presentation/logger.h"

#include <exception>
#include <stdexcept>

using namespace std;

vector<InternalMsg> get_response(
    const InternalMsgWithOriginator&,
    SyncSystem&
);
vector<InternalMsg> wrap_messages(const vector<Message>&);
vector<Message> handle_msg(const Message&, SyncSystem&);


int run_file_operator(
    const SyncConfig& config,
    ReceivingPipe<InternalMsgWithOriginator>& inbox
) {
    ExitCode exit_code;

    try {
        SyncSystem system(config);

        while (auto optional_msg{inbox.receive()}) {
            auto msg{optional_msg.value()};
            msg.originator.send(get_response(msg, system));
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

vector<InternalMsg> get_response(
    const InternalMsgWithOriginator& request,
    SyncSystem& system
) {
    switch (request.type) {
        case InternalMsgType::ServerWaits:
            return {InternalMsg(InternalMsgType::FileOperatorStarted)};
        case InternalMsgType::ClientWaits:
            return {
                InternalMsg(InternalMsgType::FileOperatorStarted),
                get_msg_to_originator(system.get_show_files())
            };
        case InternalMsgType::HandleMessage:
            return wrap_messages(handle_msg(request.msg, system));
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

vector<Message> handle_msg(const Message& request,SyncSystem& system) {
    switch (request.message_case()) {
        case Message::kShowFiles:
            return {system.get_file_list(request.show_files())};
        case Message::kFileList:
            return system.get_sync_requests(request.file_list());
        case Message::kSyncRequest:
            return {system.get_sync_response(request.sync_request())};
        case Message::kSyncResponse:
            return system.handle_sync_response(request.sync_response());
        case Message::kSignatureAddendum:
            return {system.get_sync_response(request.signature_addendum())};
        case Message::kCorrections:
            return {};
        case Message::kFileRequest:
            return {};
        case Message::kFileResponse:
            return {};
        default: 
            return {};
    }
}
