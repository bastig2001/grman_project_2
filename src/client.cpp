#include "client.h"
#include "file_operations.h"
#include "utils.h"
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

int handle_server(tcp::iostream&);
bool handle_response(const Message&);


int run_client(Config& config) {
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
                int exit_code{handle_server(server)};
                logger->info("Disconnected from server");
                return exit_code;
            }
            else {
                logger->error(
                    "Couldn't connect to server: " 
                    + server.error().message()
                );

                return 3;
            }
        }
        catch (exception& err) {
            logger->critical(
                "Following exception occurred during client execution: " 
                + string{err.what()}
            );

            return 2;
        }
    }
    else {
        return 0;
    }
}

int handle_server(tcp::iostream& server) {
    bool finished{false};
    unsigned short i{1};
    while (server && !finished) {
        Message request{};
        if (i < 4) {
            request.set_allocated_show_files(
                get_show_files({"query option 1", to_string(i)})
            );
            i++;
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
        return 4;
    }
    else {
        return 0;
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
        case Message::kGetRequest:
            break;
        case Message::kGetResponse:
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
