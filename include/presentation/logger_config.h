#pragma once

#include "logger.h"
#include "json_utils.h"

#include <json.hpp>
#include <spdlog/common.h>
#include <string>
#include <optional>


struct LoggerConfig {
    bool log_to_console{false};
    std::string file;
    spdlog::level::level_enum level_console{spdlog::level::info};
    spdlog::level::level_enum level_file{spdlog::level::info};
    size_t max_file_size{1024 * 5};
    size_t number_of_files{2};
    bool log_date{false};
    bool log_config{false};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(
        LoggerConfig, 
        log_to_console,
        file,
        level_console,
        level_file,
        max_file_size,
        number_of_files,
        log_date,
        log_config
    )

    operator std::string();
};

Logger* get_logger(const LoggerConfig&);
