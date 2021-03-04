#include "client.h"
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

            Message msg{};
            auto show = new ShowFiles;
            show->add_options()->assign("query_option 1");
            msg.set_allocated_show_files(show);
            spdlog::debug("Sending:\n{}", msg.DebugString());
            msg.SerializeToOstream(&server);

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
