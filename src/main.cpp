#include "config.h"
#include "file_operator.h"
#include "internal_msg.h"
#include "server.h"
#include "client.h"
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
    Pipe<InternalMsgWithOriginator> file_operator_inbox;

    auto server{
        config.act_as_server.has_value()
        ? async(
            launch::async, 
            bind(run_server, 
                config.act_as_server.value(), 
                ref(file_operator_inbox)
          ))
        : async(launch::deferred, [](){ return 0; })
    };

    auto client{
        config.server.has_value()
        ? async(
            launch::async, 
            bind(run_client, 
                config.server.value(), 
                ref(file_operator_inbox)
          ))
        : async(launch::deferred, [](){ return 0; })
    };

    auto file_operator{
        config.act_as_server.has_value() || config.server.has_value()
        ? async(
            launch::async, 
            bind(run_file_operator, config.sync, ref(file_operator_inbox))
          )
        : async(launch::deferred, [](){ return 0; })
    };

    int server_return = server.get();
    int client_return = client.get();
    int file_operator_return = file_operator.get();

    google::protobuf::ShutdownProtobufLibrary();

    return server_return | client_return | file_operator_return;
}
