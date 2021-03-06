#include "client.h"
#include "file_operations.h"
#include "utils.h"
#include "messages/all.pb.h"
#include "messages/info.pb.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <spdlog/spdlog.h>
#include <chrono>

using namespace std;
using namespace asio::ip;
using namespace asio;


int run_client(Config& config) {
    if (config.server.has_value()) {
        auto server_conf = config.server.value();

        tcp::iostream server{
            server_conf.address, 
            to_string(server_conf.port)
        };
        socket_base::keep_alive keep_alive;
        server.socket().set_option(keep_alive);
        server.expires_after(std::chrono::seconds{10});

        if (server) {
            spdlog::info("Connected to server");

            Message request{};
            request.set_allocated_show_files(
                get_show_files({"query option 1"})
            );
            spdlog::debug("Sending:\n{}", request.DebugString());
            server << encode_msg_base64(request) << "\n";

            Message response{decode_base64_msg_stream(server)};
            spdlog::debug("Received:\n{}", response.DebugString());

            server.close();
            return 0;
        }
        else {
            spdlog::error(
                "Couldn't connect to server: {}", 
                server.error().message()
            );

            return 2;
        }
    }
    else {
        return 0;
    }
}
