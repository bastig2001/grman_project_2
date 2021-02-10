#pragma once

#include "utils.h"

#include <variant>
#include <string>
#include <optional>


struct Server {
    std::string address;
    std::string port;

    Server(
        std::string address, 
        std::string port
    ): address{address},
       port{port}
    {}

    operator std::string() {
        return "{address: " + address + "; port: " + port + "}";
    }
};

struct Config {
    std::optional<Server> server;
    std::optional<unsigned long> local_server_port;

    operator std::string() {
        return "{server: " + optional_to_string(server) + "\n" +
               " local server port: " + optional_to_string(local_server_port) + "\n}";
    }
};

using ExitCode = int;


std::variant<ExitCode, Config> configure(int argc, char* argv[]);
