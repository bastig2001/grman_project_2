#pragma once

#include "json_utils.h"
#include "utils.h"
#include "presentation/logger_config.h"

#include <json.hpp>
#include <sstream>
#include <variant>
#include <string>
#include <optional>


struct ServerData {
    std::string address{"0.0.0.0"};
    unsigned short port{9876};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ServerData, address, port)

    operator std::string() {
        return "{\"address\": \"" + address + "\", " 
               "\"port\": " + std::to_string(port) + "}";
    }
};


struct SyncConfig {
    bool sync_hidden_files{false};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SyncConfig, sync_hidden_files)

    operator std::string() {
        std::ostringstream output{};

        output 
            << std::boolalpha
            << "{\"sync hidden files\": " << sync_hidden_files << "}";

        return output.str();
    }
};


struct Config {
    std::optional<ServerData> server;
    std::optional<ServerData> act_as_server;
    SyncConfig sync;
    LoggerConfig logger;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, server, act_as_server, sync, logger)

    operator std::string() {
        return "{\"server\":        " + optional_to_string(server, "{}")        + ",\n"
               " \"act as server\": " + optional_to_string(act_as_server, "{}") + ",\n"
               " \"sync\":          " + (std::string)sync                       + ",\n"
               " \"logger\":      \n" + (std::string)logger                     +  "\n"
               "}";
    }
};


std::variant<int, Config> configure(int argc, char* argv[]);
