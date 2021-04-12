#include "client.h"
#include "config.h"
#include "file_operator.h"
#include "internal_msg.h"
#include "server.h"
#include "utils.h"
#include "presentation/command_line.h"
#include "presentation/format_utils.h"
#include "presentation/logger.h"
#include "messages/all.pb.h"

#include <functional>
#include <future>

using namespace std;

int run(Config&);


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

        use_color = !config->no_color;

        return run(*config);
    }
    else {
        return get<int>(config_result);
    }
}

int run(Config& config) {
    Pipe<InternalMsgWithOriginator> file_operator_inbox;

    future<void> command_line;
    if (config.act_as_server.has_value() || config.server.has_value()) {
        // tart command line

        auto command_line_object = 
            new CommandLine(logger, config, file_operator_inbox);
        logger = command_line_object;
        command_line = async(launch::deferred, ref(*command_line_object));
    }
    else {
        command_line = async(launch::deferred, [](){});
    }

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
            bind(run_file_operator, config, ref(file_operator_inbox))
          )
        : async(launch::deferred, [](){ return 0; })
    };

    command_line.get();
    int return_value{file_operator.get()};

    if (is_ready(client)) {
        return_value |= client.get();
    }
    
    if (is_ready(server)) {
        return_value |= server.get();
    }

    google::protobuf::ShutdownProtobufLibrary();

    exit(return_value);
}
