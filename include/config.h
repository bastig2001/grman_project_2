#pragma once

#include "utils.h"

#include <variant>
#include <string>
#include <optional>


struct ServerData {
    std::string address{"0.0.0.0"};
    unsigned short port{9876};

    operator std::string() {
        return "{\"address\": \"" + address + "\"; " + 
               "\"port\": \"" + std::to_string(port) + "\"}";
    }
};

struct Config {
    std::optional<ServerData> server;
    std::optional<ServerData> act_as_server;

    operator std::string() {
        return "{\"server\": " + optional_to_string(server, "{}") + ";\n" +
               " \"act as server\": " + optional_to_string(act_as_server, "{}") + "\n}";
    }
};

using ExitCode = int;


std::variant<ExitCode, Config> configure(int argc, char* argv[]);
