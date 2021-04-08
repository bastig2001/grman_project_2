#pragma once

#include "logger.h"

#include <spdlog/spdlog.h>
#include <functional>
#include <mutex>


class CommandLine: public Logger {
  private:
    Logger* logger;
    std::mutex console_mtx{};

    std::function<void(spdlog::level::level_enum, const std::string&)> _log;

    void pre_output();
    void post_output();

  public:
    CommandLine(Logger*);

    ~CommandLine();

    bool logs_to_file() const override;
    bool logs_to_console() const override;

    void set_level(spdlog::level::level_enum) override;
    spdlog::level::level_enum get_level() const override;

    void set_pattern(const std::string&) override;

    void log(spdlog::level::level_enum, const std::string&) override;
    void trace(const std::string&) override;
    void debug(const std::string&) override;
    void info(const std::string&) override;
    void warn(const std::string&) override;
    void error(const std::string&) override;
    void critical(const std::string&) override;
};
