#include "server.h"
#include "messages/all.pb.h"
#include "spdlog/fmt/bundled/format.h"
#include "utils.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <sstream>
#include <streambuf>
#include <string_view>
#include <thread>

using namespace std;
using namespace asio::ip;
using namespace asio;

void handle_client(tcp::iostream&&);


int run_server(Config& config) {
    if (config.act_as_server.has_value()) {
        auto serve_conf = config.act_as_server.value();

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
            spdlog::critical(
                "Following exception occurred during server execution: {}", 
                err.what()
            );

            return 1;
        }
        
    }
    else {
        return 0;
    }
}

void handle_client(tcp::iostream&& client) {
    if (client) {
        spdlog::debug("Client connected");

        Message msg{decode_base64_msg_stream(client)};
        spdlog::debug("Received:\n{}", msg.DebugString());

        client.close();
    }
    else {
        spdlog::error(
            "Following connection error occurred: {}",
            client.error().message()
        );
    }
}
