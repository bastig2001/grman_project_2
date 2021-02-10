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

    operator std::string() {
        return "  {address: " + address + "\n" +
               "   port: "    + port    + "\n  }";
    }
};

struct Config {
    std::optional<Server> server;
    std::optional<unsigned long> local_server_port;

    operator std::string() {
        return "{server: \n" + 
               (server.has_value() 
                    ? (std::string)server.value() 
                    : "  None"
               ) + "\n" +
               " local server port: " + 
               (local_server_port.has_value()
                    ? std::to_string(local_server_port.value())
                    : "None"
               ) + "\n}";
    }
};

using ExitCode = int;


std::variant<ExitCode, Config> configure(int argc, char* argv[]);
