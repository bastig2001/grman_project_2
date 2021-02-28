#include "client.h"
#include "messages/all.pb.h"
#include "messages/info.pb.h"

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

            Message msg{};
            msg.set_allocated_show_files(new ShowFiles);
            server << msg.SerializeAsString();
            msg.Clear();
            
            if (server) {
                string buffer{};
                getline(server, buffer);
                if (msg.ParseFromString(buffer)) {
                    switch (msg.message_case()) {
                        case Message::kShowFiles:
                            spdlog::info("Got a request to show all files");
                            break;
                        case Message::kFileList:
                            spdlog::info("Got a list of files");
                            break;
                        case Message::kGetRequest:
                            spdlog::info("Received a GET-request");
                            break;
                        case Message::kGetResponse:
                            spdlog::info("Received a GET-response");
                            break;
                        case Message::kSyncRequest:
                            spdlog::info("Received a SYNC-request");
                            break;
                        case Message::kSyncResponse:
                            spdlog::info("Received a SYNC-response");
                            break;
                        case Message::MESSAGE_NOT_SET:
                            spdlog::info("Received an undefined message");
                            break;
                    }
                    spdlog::debug("Received:\n{}", msg.DebugString());
                }
            }
            else {
                spdlog::error(
                    "Couldn't connect to server: {}", 
                    server.error().message()
                );
                return server.error().value();
            }

            return 0;
        }
        else {
            spdlog::error(
                "Couldn't connect to server: {}", 
                server.error().message()
            );
            return server.error().value();
        }
    }
    else {
        return 0;
    }
}
