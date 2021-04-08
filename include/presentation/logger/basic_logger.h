#pragma once

#include "presentation/logger.h"

#include <spdlog/spdlog.h>


// A basic implementation of Logger which uses spdlog
class BasicLogger: public Logger {
  private:
    std::shared_ptr<spdlog::logger> logger;
    bool is_file_logger;
    
  public:
    BasicLogger(
        std::shared_ptr<spdlog::logger> logger,
        bool is_file_logger
    ): logger{logger},
       is_file_logger{is_file_logger}
    {}

    bool logs_to_file() const override { 
        return is_file_logger && logger->level() != spdlog::level::off; 
    }
    bool logs_to_console() const override { 
        return !is_file_logger && logger->level() != spdlog::level::off; 
    }

    void set_level(spdlog::level::level_enum level) override {
        logger->set_level(level);
    }
    spdlog::level::level_enum get_level() const override { 
        return logger->level(); 
    }

    void set_pattern(const std::string& pattern) override {
        logger->set_pattern(pattern);
    }

    void log(spdlog::level::level_enum level, const std::string& msg) override { 
        logger->log(level, msg);
    }
    void trace(const std::string& msg) override {
        logger->trace(msg);
    }
    void debug(const std::string& msg) override {
        logger->debug(msg);
    }
    void info(const std::string& msg) override {
        logger->info(msg);
    }
    void warn(const std::string& msg) override {
        logger->warn(msg);
    }
    void error(const std::string& msg) override {
        logger->error(msg);
    }
    void critical(const std::string& msg) override {
        logger->critical(msg);
    }
};
