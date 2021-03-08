#pragma once

#include "utils.h"

#include <spdlog/common.h>
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

struct LogConfig {
    bool log_to_console{false};
    std::optional<std::string> file;
    spdlog::level::level_enum level_console{spdlog::level::info};
    spdlog::level::level_enum level_file{spdlog::level::info};
    bool log_date{false};
    bool log_config{false};
    size_t max_file_size{1024 * 5};
    size_t number_of_files{2};

    operator std::string();
};

struct Config {
    std::optional<ServerData> server;
    std::optional<ServerData> act_as_server;
    LogConfig log;

    operator std::string() {
        return "{\"server\":        " + optional_to_string(server, "{}")        + ";\n"
               " \"act as server\": " + optional_to_string(act_as_server, "{}") + ";\n"
               " \"log\":         \n" + (std::string)log                        +  "\n"
               "}";
    }
};

using ExitCode = int;


std::variant<ExitCode, Config> configure(int argc, char* argv[]);
