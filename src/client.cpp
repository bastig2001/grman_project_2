#include "client.h"
#include "config.h"
#include "file_operations.h"
#include "utils.h"
#include "exit_code.h"
#include "presentation/logger.h"
#include "messages/all.pb.h"
#include "messages/info.pb.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <chrono>
#include <tuple>

using namespace std;
using namespace asio::ip;
using namespace asio;

int handle_server(tcp::iostream&, const Config&);
bool handle_response(const Message&);


int run_client(const Config& config) {
    if (config.server.has_value()) {
        auto server_conf = config.server.value();

        try {
            tcp::iostream server{
                server_conf.address, 
                to_string(server_conf.port)
            };
            socket_base::keep_alive keep_alive;
            server.socket().set_option(keep_alive);
            server.expires_after(std::chrono::seconds{10});

            if (server) {
                logger->info("Connected to server");
                int exit_code{handle_server(server, config)};
                logger->info("Disconnected from server");
                return exit_code;
            }
            else {
                logger->error(
                    "Couldn't establish connection to server: " 
                    + server.error().message()
                );

                return ConnectionEstablishmentError;
            }
        }
        catch (exception& err) {
            logger->critical(
                "Following exception occurred during client execution: " 
                + string{err.what()}
            );

            return ClientException;
        }
    }
    else {
        return Success;
    }
}

int handle_server(tcp::iostream& server, const Config& config) {
    auto files{get_files(config.sync_hidden_files)};
    unsigned int file_index{0};
    logger->debug("Files to sync:\n" + vector_to_string(files, "\n"));

    bool finished{false};
    while (server && !finished) {
        Message request{};
        if (file_index < files.size()) {
            request.set_allocated_check_file_request(
                get_check_file_request(get_file(files[file_index]))
            );
            file_index++;
        }
        else {
            request.set_finish(true);
        }
        logger->debug("Sending:\n" + request.DebugString());
        server << msg_to_base64(request) << "\n";

        if (server) {
            Message response{msg_from_base64(server)};
            logger->debug("Received:\n" + response.DebugString());
            
            finished = handle_response(response);
            if (finished) {
                server.close();
            }
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

bool handle_response(const Message& response) {
    bool finish{false};

    switch (response.message_case()) {
        case Message::kShowFiles:
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
        case Message::kReceived:
            break;
        case Message::kFinish:
            finish = true;
            break;
        case Message::MESSAGE_NOT_SET:
            logger->warn("Received an undefined message");
            break;
    }

    return finish;
}
