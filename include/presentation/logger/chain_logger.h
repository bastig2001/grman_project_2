#pragma once

#include "presentation/logger.h"


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
