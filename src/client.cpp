#include "client.h"

#include <asio.hpp>
#include <chrono>
#include <spdlog/spdlog.h>

using namespace asio::ip;
using namespace std;


int run_client(Config config) {
    if (config.server.has_value()) {
        auto server_conf = config.server.value();

        tcp::iostream server{server_conf.address, to_string(server_conf.port)};
        server.expires_after(chrono::seconds{10});

        if (server) {
            spdlog::info("Connected to server");
        }
        else {
            spdlog::error(
                "Couldn't connect to server: {}", 
                server.error().message()
            );
        }
    }

    return 0;
}
