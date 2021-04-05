#include "client.h"
#include "config.h"
#include "internal_msg.h"
#include "pipe.h"
#include "utils.h"
#include "exit_code.h"
#include "presentation/logger.h"
#include "messages/all.pb.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <chrono>
#include <tuple>

using namespace std;
using namespace asio::ip;
using namespace asio;

bool wait_for(SendingPipe<InternalMsgWithOriginator>&, Pipe<InternalMsg>&);
ExitCode handle_server(
    tcp::iostream&, 
    SendingPipe<InternalMsgWithOriginator>&, 
    Pipe<InternalMsg>&
);
bool handle_response(
    const Message&, 
    SendingPipe<InternalMsgWithOriginator>&, 
    SendingPipe<InternalMsg>&
);


int run_client(
    const ServerData& config,
    SendingPipe<InternalMsgWithOriginator>& file_operator
) {
    ExitCode exit_code;
    Pipe<InternalMsg> inbox;

    try {
        if (wait_for(file_operator, inbox)) {
            tcp::iostream server{
                config.address, 
                to_string(config.port)
            };
            socket_base::keep_alive keep_alive;
            server.socket().set_option(keep_alive);

            if (server) {
                logger->info("Connected to server");
                exit_code = handle_server(server, file_operator, inbox);
                logger->info("Disconnected from server");
            }
            else {
                logger->error(
                    "Couldn't establish connection to server: " 
                    + server.error().message()
                );

                exit_code = ConnectionEstablishmentError;
            }
        }
        else {
            exit_code = FileOperatorNotStarted;
        }  
    }
    catch (exception& err) {
        logger->critical(
            "Following exception occurred during client execution: " 
            + string{err.what()}
        );

        exit_code = ClientException;
    }

    inbox.close();
    file_operator.close();
    return exit_code;
}

bool wait_for(
    SendingPipe<InternalMsgWithOriginator>& file_operator, 
    Pipe<InternalMsg>& inbox
) {
    file_operator.send(
        InternalMsgWithOriginator(InternalMsgType::ClientWaits, inbox)
    );

    if (auto response{inbox.receive()}) {
        return response.value().type == InternalMsgType::FileOperatorStarted;
    }
    else {
        return false;
    }
}

ExitCode handle_server(
    tcp::iostream& server, 
    SendingPipe<InternalMsgWithOriginator>& file_operator,
    Pipe<InternalMsg>& inbox
) {
    bool finished{false};
    while (server && !finished) {
        if (auto optional_msg{inbox.receive()}) {
            auto msg{optional_msg.value()};

            logger->debug("Sending:\n" + msg.msg.DebugString());
            server << msg_to_base64(msg.msg) << "\n";

            if (server) {
                Message response{msg_from_base64(server)};
                logger->debug("Received:\n" + response.DebugString());
                
                finished = handle_response(response, file_operator, inbox);
                if (finished || file_operator.is_closed()) {
                    server.close();
                }
            }  
        }
        else {
            break;
        }
    }

    if (server.error()) {
        logger->error(
            "Following connection error occurred: "
            + server.error().message()
        );
        return ConnectionError;
    }
    else {
        return Success;
    }
}

bool handle_response(
    const Message& response, 
    SendingPipe<InternalMsgWithOriginator>& file_operator,
    SendingPipe<InternalMsg>& inbox
) {
    bool finish{false};

    switch (response.message_case()) {
        case Message::kFinish:
            finish = true;
            break;
        case Message::MESSAGE_NOT_SET:
            logger->warn("Received an undefined message");
            break;
        default:
            file_operator.send(get_msg_to_file_operator(inbox, response));
            break;
    }

    return finish;
}
