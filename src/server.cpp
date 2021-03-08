#include "server.h"
#include "file_operations.h"
#include "utils.h"
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

void handle_client(tcp::iostream&&);
tuple<Message, bool> get_response(const Message&);


int run_server(Config& config) {
    if (config.act_as_server.has_value()) {
        auto serve_conf{config.act_as_server.value()};

        try {
            io_context io_ctx;
            tcp::endpoint tcp_ep{
                make_address(serve_conf.address), 
                serve_conf.port
            };
            tcp::acceptor acceptor{io_ctx, tcp_ep};

            acceptor.listen();

            socket_base::keep_alive keep_alive;

            while (true) {
                tcp::socket socket{io_ctx};
                acceptor.accept(socket);
                socket.set_option(keep_alive);
                tcp::iostream client{move(socket)};
                client.expires_after(std::chrono::seconds{5});

                thread{handle_client, move(client)}.detach();
            }

            return 0;
        }
        catch (exception& err) {
            logger->critical(
                "Following exception occurred during server execution: " 
                + string{err.what()}
            );

            return 1;
        }
        
    }
    else {
        return 0;
    }
}

void handle_client(tcp::iostream&& client) {
    logger->info("Client connected");

    bool finished{false};
    while (client && !finished) {
        Message request{msg_from_base64(client)};
        logger->debug("Received:\n" + request.DebugString());

        if (client) {
            auto [response, finish]{get_response(request)};
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

tuple<Message, bool> get_response(const Message& request) {
    Message response{};
    bool finish{false};

    switch (request.message_case()) {
        case Message::kShowFiles:
            response.set_allocated_file_list(
                show_files(request.show_files())
            );
            break;
        case Message::kFileList:
            response.set_received(true);
            break;
        case Message::kSyncRequest:
            response.set_allocated_sync_response(
                get_sync_response(request.sync_request())
            );
            break;
        case Message::kSyncResponse:
            response.set_received(true);
            break;
        case Message::kGetRequest:
            response.set_allocated_get_response(
                get_get_response(request.get_request())
            );
            break;
        case Message::kGetResponse:
            response.set_received(true);
            break;
        case Message::kReceived:
            response.set_received(true);
            break;
        case Message::kFinish:
            response.set_finish(true);
            finish = true;
            break;
        case Message::MESSAGE_NOT_SET:
            logger->warn("Received an undefined message");
            break;
    }

    return {response, finish};
}
