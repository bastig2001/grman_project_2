#pragma once

#include "config.h"
#include "internal_msg.h"
#include "logger.h"
#include "pipe.h"

#include <fmt/core.h>
#include <peglib.h>
#include <spdlog/spdlog.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>


class CommandLine: public Logger {
  private:
    const std::string prompt{"> "};
    const size_t prompt_length{prompt.length()};
    const unsigned int max_input_history_size{100};

    Logger* logger;
    Config& config;
    SendingPipe<InternalMsgWithOriginator>& file_operator;

    std::mutex console_mtx{};

    std::function<void(spdlog::level::level_enum, const std::string&)> _log;

    peg::parser command_parser;
    void init_command_parser();

    void execute(const std::string&);

    void print_error(size_t, size_t, const std::string&);
    void help();
    void list();
    void list_long();
    void sync();
    void exit();

    bool in_esc_mode{false};
    std::string ctrl_sequence{};
    unsigned int next_input_history_index{0};
    std::vector<std::string> input_history{};
    std::string original_input{""};
    std::string current_input{""};
    unsigned int cursor_position{0};

    void handle_input(char);
    void handle_input_in_esc_mode(char);
    void handle_input_in_regular_mode(char);
    void show_history_up();
    void show_history_down();
    void update_input(const std::string&);
    void move_cursor_right();
    void move_cursor_left();
    void do_delete();
    void do_backspace();
    void handle_newline();
    void update_input_history(const std::string&);
    void write_char(char);
    void write_user_input(unsigned int = 0);

    void clear_line();
    void print_prompt_and_user_input();

    std::function<void()> pre_output{
        std::bind(&CommandLine::clear_line, this)
    };
    std::function <void()> post_output{
        std::bind(&CommandLine::print_prompt_and_user_input, this)
    };

    template<typename... Args>
    void println(const Args&... args) {
        std::lock_guard console_lck{console_mtx};
        pre_output();
        fmt::print(args...);
        fmt::print("\n");
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
    CommandLine(Logger*, Config&, SendingPipe<InternalMsgWithOriginator>&);

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
