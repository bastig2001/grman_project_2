#include "config.h"

#include <spdlog/spdlog.h>

using namespace std;


int main(int argc, char* argv[]) {
    auto config_result = configure(argc, argv);
    spdlog::set_level(spdlog::level::debug);
    
    if (auto config = get_if<Config>(&config_result)) {
        spdlog::debug(
            "This program was called correctly.\n"
            "The config is:\n{}", (string)*config
        );
    }
    else {
        return get<int>(config_result);
    }
}

