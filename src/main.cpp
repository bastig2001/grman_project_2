#include "config.h"
#include "file_operator.h"
#include "server.h"
#include "client.h"
#include "pipe/basic_pipe.h"
#include "pipe/no_pipe.h"
#include "presentation/logger.h"
#include "messages/all.pb.h"

#include <functional>
#include <future>

using namespace std;

int run(const Config&);


int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    auto config_result = configure(argc, argv);
    
    if (auto config{get_if<Config>(&config_result)}) {
        logger = get_logger(config->logger);

        if (config->logger.log_config) {
            logger->debug(
                "This program was called correctly.\n"
                "The config is:\n" + (string)*config
            );
        }

        return run(*config);
    }
    else {
        return get<int>(config_result);
    }
}

int run(const Config& config) {
    BasicPipe file_operator_inbox;
    Pipe* server_inbox;
    Pipe* client_inbox;
    future<int> server;
    future<int> client;

    if (config.act_as_server.has_value()) {
        // server is used
        server_inbox = new BasicPipe;
        server = 
            async(
                launch::async, 
                bind(run_server, config, server_inbox, &file_operator_inbox)
            );
    }
    else {
        // server is not used
        server_inbox = new NoPipe;
        server = async(launch::deferred, [](){ return 0; });
    }

    if (config.server.has_value()) {
        // client is used
        client_inbox = new BasicPipe;
        client = 
            async(
                launch::async, 
                bind(run_client, config, client_inbox, &file_operator_inbox)
            );
    }
    else {
        // client is not used
        client_inbox = new NoPipe;
        client = async(launch::deferred, [](){ return 0; });
    }

    auto file_operator = 
        async(
            launch::async, 
            bind(run_file_operator, 
                config, 
                &file_operator_inbox, 
                server_inbox, 
                client_inbox
        ));

    int server_return = server.get();
    int client_return = client.get();
    int file_operator_return = file_operator.get();

    google::protobuf::ShutdownProtobufLibrary();

    return server_return | client_return | file_operator_return;
}
