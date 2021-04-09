#pragma once

#include "presentation/logger.h"


// An implementation of Logger which does nothing
class NoLogger: public Logger {
  public:
    bool logs_to_file() const override { return false; }
    bool logs_to_console() const override { return false; }

    void set_level(spdlog::level::level_enum) override {}
    spdlog::level::level_enum get_level() const override { return spdlog::level::off; }

    void set_pattern(const std::string&) override {}

    void log(spdlog::level::level_enum, const std::string&) override {}
    void trace(const std::string&) override {}
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
    void critical(const std::string&) override {}
};
