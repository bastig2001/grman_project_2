#pragma once

#include "json_utils.h"
#include "utils.h"
#include "presentation/logger_config.h"

#include <json.hpp>
#include <ios>
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
    bool no_color;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, server, act_as_server, sync, logger)

    operator std::string() {
        std::ostringstream output{};
        output
            << std::boolalpha
            << "{\"server\":        " << optional_to_string(server, "{}")        << ",\n"
            << " \"act as server\": " << optional_to_string(act_as_server, "{}") << ",\n"
            << " \"sync\":          " << (std::string)sync                       << ",\n"
            << " \"logger\":      \n" << (std::string)logger                     << ",\n"
            << " \"no color\":      " << no_color                                << " \n"
            << "}";
        
        return output.str();
    }
};


std::variant<int, Config> configure(int argc, char* argv[]);
