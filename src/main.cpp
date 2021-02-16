#include "config.h"
#include "server.h"
#include "client.h"

#include <spdlog/spdlog.h>
#include <future>

using namespace std;

int run(const Config&);


int main(int argc, char* argv[]) {
    auto config_result = configure(argc, argv);
    spdlog::set_level(spdlog::level::debug);
    
    if (auto config{get_if<Config>(&config_result)}) {
        spdlog::debug(
            "This program was called correctly.\n"
            "The config is:\n{}", (string)*config
        );

        return run(*config);
    }
    else {
        return get<int>(config_result);
    }
}

int run(const Config& config) {
    auto server{async(launch::async, run_server, config)};
    auto client{async(launch::async, run_client, config)};

    int server_return = server.get();
    int client_return = client.get();

    return server_return | client_return;
}
