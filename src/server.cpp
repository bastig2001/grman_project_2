#include "server.h"

#include <asio.hpp>
#include <chrono>
#include <spdlog/spdlog.h>

using namespace std;
using namespace asio::ip;
using namespace asio;


int run_server(Config config) {
    if (config.act_as_server.has_value()) {
        auto serve_conf = config.act_as_server.value();

        io_context ip_ctx;
        tcp::endpoint tcp_ep{make_address(serve_conf.address), serve_conf.port};
        tcp::acceptor acceptor{ip_ctx, tcp_ep};

        acceptor.listen();

        while (true) {
            tcp::socket socket{ip_ctx};
            acceptor.accept(socket);
            tcp::iostream client{move(socket)};
            client.expires_after(std::chrono::seconds{10});

            if (client) {
                spdlog::info("Connected to client");
            }
            else {
                spdlog::error(
                    "Couldn't connect to client: {}", 
                    client.error().message()
                );
            }
        }
    }

    return 0;
}
