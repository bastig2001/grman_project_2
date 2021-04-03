#include "server.h"
#include "config.h"
#include "file_operations.h"
#include "internal_msg.h"
#include "utils.h"
#include "exit_code.h"
#include "presentation/logger.h"
#include "messages/all.pb.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <chrono>
#include <sstream>
#include <streambuf>
#include <string_view>
#include <thread>
#include <tuple>

using namespace std;
using namespace asio::ip;
using namespace asio;

bool wait_for(SendingPipe<InternalMsg>*);
void handle_client(tcp::iostream&&, SendingPipe<InternalMsg>*);
tuple<Message, bool> get_response(const Message&, SendingPipe<InternalMsg>*);
tuple<Message, bool> get_response_from(SendingPipe<InternalMsg>*, const Message&);


int run_server(
    const ServerData& config,
    SendingPipe<InternalMsg>* file_operator
) {
    ExitCode exit_code;

    try {
        io_context io_ctx;
        tcp::endpoint tcp_ep{
            make_address(config.address), 
            config.port
        };
        tcp::acceptor acceptor{io_ctx, tcp_ep};

        acceptor.listen();

        socket_base::keep_alive keep_alive;

        if (wait_for(file_operator)) {
            while (file_operator->is_open()) {
                tcp::socket socket{io_ctx};
                acceptor.accept(socket);
                socket.set_option(keep_alive);
                tcp::iostream client{move(socket)};
                client.expires_after(std::chrono::seconds{5});

                thread{handle_client, move(client), file_operator}.detach();
            }

            exit_code = Success;
        }
        else {
            logger->critical("File operator hasn't started");

            exit_code = FileOperatorNotStarted;
        }        
    }
    catch (exception& err) {
        logger->critical(
            "Following exception occurred during server execution: " 
            + string{err.what()}
        );

        exit_code = ServerException;
    }

    file_operator->close();
    return exit_code;
}

bool wait_for(SendingPipe<InternalMsg>* file_operator) {
    Pipe<InternalMsg> responder;
    file_operator->send(InternalMsg(InternalMsgType::ServerWaits, &responder));

    if (auto optional_response{responder.receive()}) {
        auto response{optional_response.value()};
        return response.type == InternalMsgType::FileOperatorStarted;
    }
    else {
        return false;
    }
}

void handle_client(
    tcp::iostream&& client, 
    SendingPipe<InternalMsg>* file_operator
) {
    logger->info("Client connected");

    bool finished{false};
    while (client && !finished) {
        Message request{msg_from_base64(client)};
        logger->debug("Received:\n" + request.DebugString());

        if (client) {
            auto [response, finish]{get_response(request, file_operator)};
            logger->debug("Sending:\n" + response.DebugString());
            client << msg_to_base64(response) << "\n";

            if (finish) {
                client.close();
                finished = true;
            }
        }
    }

    if (client.error()) {
        logger->error(
            "Following connection error occurred: "
            + client.error().message()
        );
    }
    if (finished) {
        logger->info("Client disconnected");
    }
}

tuple<Message, bool> get_response(
    const Message& request, 
    SendingPipe<InternalMsg>* file_operator
) {
    Message response{};
    bool finish{false};

    switch (request.message_case()) {
        case Message::kFinish:
            response.set_finish(true);
            finish = true;
            break;
        case Message::MESSAGE_NOT_SET:
            logger->warn("Received an undefined message");
            break;
        default:
            auto [file_operator_response, file_operator_success] = 
                get_response_from(file_operator, request);

            if (file_operator_success) {
                response = file_operator_response;
            }
            else {
                response.set_finish(true);
                finish = true;
            }

            break;
    }

    return {response, finish};
}

tuple<Message, bool> get_response_from(
    SendingPipe<InternalMsg>* file_operator, 
    const Message& request
) {
    Pipe<InternalMsg> responder;
    file_operator->send(InternalMsg::to_file_operator(&responder, request));

    if (auto optional_msg{responder.receive()}) {
        auto msg{optional_msg.value()};
        return {msg.msg, msg.type == InternalMsgType::SendMessage};
    }
    else {
        return {Message{}, false};
    }
}
