#pragma once

#include "config.h"
#include "logger.h"

#include <peglib.h>
#include <spdlog/spdlog.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>


class CommandLine: public Logger {
  private:
    const std::string prompt{"> "};
    const size_t prompt_length{prompt.length()};

    Logger* logger;
    Config& config;

    std::mutex console_mtx{};

    std::function<void(spdlog::level::level_enum, const std::string&)> _log;

    void pre_output();
    void post_output();

    peg::parser command_parser;
    void init_command_parser();

    void execute(const std::string&);

    void print_error(size_t, size_t, const std::string&);
    void help();
    void list();
    void list_long();
    void exit();

    template<typename... Args>
    void print(const Args&... args) {
        std::lock_guard console_lck{console_mtx};
        pre_output();
        (std::cout << ... << args);
        post_output();
    }

    template<typename... Args>
    void println(const Args&... args) {
        std::lock_guard console_lck{console_mtx};
        pre_output();
        (std::cout << ... << args) << std::endl;
        post_output();
    }

    template<typename... Args>
    void eprintln(const Args&... args) {
        std::lock_guard console_lck{console_mtx};
        pre_output();
        (std::cerr << ... << args) << std::endl;
        post_output();
    }

  public:
    CommandLine(Logger*, Config&);

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

    void operator()();
};
