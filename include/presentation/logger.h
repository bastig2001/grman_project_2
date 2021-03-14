#pragma once

#include <spdlog/spdlog.h>
#include <algorithm>
#include <memory>
#include <string>


// The interface for any Logger
class Logger {
  public:
    virtual bool logs_to_file() const = 0;

    virtual void set_level(spdlog::level::level_enum) = 0;
    virtual spdlog::level::level_enum get_level() = 0;

    virtual void set_pattern(const std::string&) = 0;

    virtual void log(spdlog::level::level_enum, const std::string&) = 0;
    virtual void trace(const std::string&) = 0;
    virtual void debug(const std::string&) = 0;
    virtual void info(const std::string&) = 0;
    virtual void warn(const std::string&) = 0;
    virtual void error(const std::string&) = 0;
    virtual void critical(const std::string&) = 0;

    virtual ~Logger() = default;
};

// Global Logger object for all of the logging
extern Logger* logger;
