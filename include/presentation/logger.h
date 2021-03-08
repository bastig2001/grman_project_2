#pragma once

#include "config.h"

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


// An implementation of Logger which does nothing
class NoLogger: public Logger {
  public:
    bool logs_to_file() const override { return true; }

    void set_level(spdlog::level::level_enum) override {}
    spdlog::level::level_enum get_level() override { return spdlog::level::off; }

    void set_pattern(const std::string&) override {}

    void log(spdlog::level::level_enum, const std::string&) override {}
    void trace(const std::string&) override {}
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
    void critical(const std::string&) override {}
};


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
        return is_file_logger; 
    }

    void set_level(spdlog::level::level_enum level) override {
        logger->set_level(level);
    }
    spdlog::level::level_enum get_level() override { 
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


// A Logger implementation which can chain multiple Logger objects together
class ChainLogger: public Logger {
  private:
    Logger* logger1;
    Logger* logger2;

  public:
    ChainLogger(
        Logger* logger1, 
        Logger* logger2
    ): logger1{logger1},
       logger2{logger2}
    {}

    bool logs_to_file() const override { 
        return logger1->logs_to_file() || logger2->logs_to_file(); 
    }

    void set_level(spdlog::level::level_enum level) override {
        logger1->set_level(level);
        logger2->set_level(level);
    }
    spdlog::level::level_enum get_level() override { 
        return std::min(logger1->get_level(), logger2->get_level());
    }

    void set_pattern(const std::string& pattern) override {
        logger1->set_pattern(pattern);
        logger2->set_pattern(pattern);
    }

    void log(spdlog::level::level_enum level, const std::string& msg) override { 
        logger1->log(level, msg);
        logger2->log(level, msg);
    }
    void trace(const std::string& msg) override {
        logger1->trace(msg);
        logger2->trace(msg);
    }
    void debug(const std::string& msg) override {
        logger1->debug(msg);
        logger2->debug(msg);
    }
    void info(const std::string& msg) override {
        logger1->info(msg);
        logger2->info(msg);
    }
    void warn(const std::string& msg) override {
        logger1->warn(msg);
        logger2->warn(msg);
    }
    void error(const std::string& msg) override {
        logger1->error(msg);
        logger2->error(msg);
    }
    void critical(const std::string& msg) override {
        logger1->critical(msg);
        logger2->critical(msg);
    }

    ~ChainLogger() {
        delete logger1;
        delete logger2;
    }
};


Logger* get_logger(const LogConfig&);
