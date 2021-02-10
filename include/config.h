#pragma once

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
};

struct Config {
    std::optional<Server> server;
    std::optional<unsigned long> local_server_port;
};

using ExitCode = int;


std::variant<ExitCode, Config> configure(int argc, char* argv[]);
