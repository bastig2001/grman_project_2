#include "server.h"
#include "messages/all.pb.h"
#include "messages/info.pb.h"

#include <asio.hpp>
#include <chrono>
#include <cstdio>
#include <google/protobuf/message.h>
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
            client.expires_after(std::chrono::seconds{5});

            if (client) {
                spdlog::info("Connected to client");

                Message msg{};
                auto files = new FileList;
                auto file = files->add_files();
                file->set_file_name("Test-File");
                file->set_signature("abc");
                file->set_timestamp(42);
                if (msg.ParseFromIstream(&client)) {
                    switch (msg.message_case()) {
                        case Message::kShowFiles:
                            spdlog::info("Got a request to show all files");
                            msg.set_allocated_file_list(files);
                            spdlog::debug("Response to Client:\n{}", msg.DebugString());
                            if (client) {                           
                                client << msg.SerializeAsString();
                            }
                            else {
                                spdlog::error(
                                    "Couldn't connect to client: {}", 
                                    client.error().message()
                                );
                            }
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
                } 
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
