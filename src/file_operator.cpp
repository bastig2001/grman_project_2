#include "file_operator.h"
#include "config.h"
#include "exit_code.h"
#include "internal_msg.h"
#include "message_utils.h"
#include "pipe.h"
#include "file_operator/sync_system.h"
#include "messages/all.pb.h"
#include "presentation/logger.h"

#include <exception>
#include <functional>
#include <future>

using namespace std;

int run_file_operator_worker(
    SyncSystem&, 
    ReceivingPipe<InternalMsgWithOriginator>&
);
vector<InternalMsg> get_response(
    const InternalMsgWithOriginator&,
    SyncSystem&
);
vector<InternalMsg> wrap_messages(const vector<Message>&);
vector<Message> handle_msg(const Message&, SyncSystem&);


int run_file_operator(
    const Config& config,
    ReceivingPipe<InternalMsgWithOriginator>& inbox
) {
    int exit_code;
    size_t number_of_workers{4};
    vector<future<int>> workers{};
    workers.reserve(number_of_workers);

    try {
        SyncSystem system(config);

        for (size_t i{0}; i < number_of_workers; i++) {
            workers.push_back(async(
                launch::async,
                bind(run_file_operator_worker, ref(system), ref(inbox))
            ));
        }

        logger->debug("All File Operator workers are started");

        exit_code = Success;
        
        for (auto& worker: workers) {
            exit_code |= worker.get();
        }
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

int run_file_operator_worker(
    SyncSystem& system, 
    ReceivingPipe<InternalMsgWithOriginator>& inbox
) {
    ExitCode exit_code;

    try {
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
            return {system.correct(request.corrections())};
        case Message::kFileRequest:
            return {system.get_file(request.file_request().file())};
        case Message::kFileResponse:
            return {system.create_file(request.file_response())};
        default: 
            return {};
    }
}
