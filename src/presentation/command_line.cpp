#include "presentation/command_line.h"
#include "presentation/format_utils.h"
#include "config.h"
#include "utils.h"
#include "file_operator/filesystem.h"

#include <fmt/core.h>
#include <fmt/color.h>
#include <peglib.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <termios.h>

using namespace std;
using namespace placeholders;


CommandLine::CommandLine(
    Logger* logger, 
    Config& config,
    SendingPipe<InternalMsgWithOriginator>& file_operator
): logger{logger},
   config{config},
   file_operator{file_operator}
{
    if (logger->logs_to_console()) {
        _log = [this](spdlog::level::level_enum level, const std::string& msg){
            lock_guard console_lck{this->console_mtx};
            pre_output();
            this->logger->log(level, msg);
            post_output();
        };
    }
    else {
        _log = [this](spdlog::level::level_enum level, const std::string& msg){
            this->logger->log(level, msg);
        };
    }

    init_command_parser();
}

CommandLine::~CommandLine() { 
    delete logger; 
}

bool CommandLine::logs_to_file() const { 
    return logger->logs_to_file(); 
}

bool CommandLine::logs_to_console() const { 
    return logger->logs_to_console(); 
}

void CommandLine::set_level(spdlog::level::level_enum level) { 
    logger->set_level(level);
}

spdlog::level::level_enum CommandLine::get_level() const {
    return logger->get_level();
}

void CommandLine::set_pattern(const string& pattern) {
    logger->set_pattern(pattern);
}

void CommandLine::log(spdlog::level::level_enum level, const string& msg) {
    _log(level, msg);
}

void CommandLine::trace(const string& msg) {
    _log(spdlog::level::trace, msg);
}

void CommandLine::debug(const string& msg) {
    _log(spdlog::level::debug, msg);
}

void CommandLine::info(const string& msg) {
    _log(spdlog::level::info, msg);
}

void CommandLine::warn(const string& msg) {
    _log(spdlog::level::warn, msg);
}

void CommandLine::error(const string& msg) {
    _log(spdlog::level::err, msg);
}

void CommandLine::critical(const string& msg) {
    _log(spdlog::level::critical, msg);
}


void CommandLine::init_command_parser() {
    command_parser = {R"(
        Procedure <- Help / ListLong / List / Exit
        Help      <- 'help'i / 'h'i
        ListLong  <- 'list long'i / 'll'i
        List      <- 'list'i / 'ls'i
        Exit      <- 'quit'i / 'q'i / 'exit'i

        %whitespace <- [ \t]*
    )"};

    command_parser.log = 
        bind(&CommandLine::print_error, this, ::_1, ::_2, ::_3);

    command_parser["Help"] = 
        [this](const peg::SemanticValues&){ help(); };
    command_parser["List"] = 
        [this](const peg::SemanticValues&){ list(); };
    command_parser["ListLong"] = 
        [this](const peg::SemanticValues&){ list_long(); };
    command_parser["Exit"] = 
        [this](const peg::SemanticValues&){ exit(); };
}

void CommandLine::execute(const std::string& command) {
    command_parser.parse(command.c_str());
}

void CommandLine::print_error(
    size_t, // ignored
    size_t column, 
    const std::string& err_msg
) {
    unsigned long err_position{prompt_length + column - 1};
    string output(err_position, ' ');
    string msg{err_msg + " starting here "};

    output += "^\n" + msg;

    if (err_position > msg.size()) {
        output += string(err_position - msg.size(), '_') + "|";
    }

    output += "\nRun 'help' for more information.";

    eprintln(output);
}

void CommandLine::help() {
    println(
        "Following Commands are available:\n"
        "  h, help        outputs this help message\n"
        "  ls, list       lists all files which are to be synced \n"
        "  ll, list long  lists all files which are to be synced with more information \n"
        "  q, quit, exit  quits and exits the program\n"
    );
}

void CommandLine::list() {
    string output{""};
    for (auto& path: fs::get_file_paths(config.sync.sync_hidden_files)) {
        output += path.string() + "\n";
    }

    println(output);
}

void CommandLine::list_long() {
    string output{
        "signature" + string(32 + 2 - 9, ' ') +  
        fmt::format(fg(fmt::color::sea_green), "size      ") +
        fmt::format(fg(fmt::color::cadet_blue), "last changed         ") +
        fmt::format(fg(fmt::color::burly_wood), "path\n\n")
    };

    for (auto file: 
            fs::get_files(fs::get_file_paths(config.sync.sync_hidden_files))
    ) {
        output += format_file(*file) + "\n";
        
        delete file;
    }

    println(output);
}

void CommandLine::exit() {
    file_operator.close();
    pre_output = [](){};
    post_output = [](){};

    println("\nexited");
}


void CommandLine::operator()() {
    struct termios original_stdin_settings{}, new_stdin_settings{};

    // save stdin settings
    tcgetattr(fileno(stdin), &original_stdin_settings); 

    new_stdin_settings = original_stdin_settings;

    // disbale echoing in new stdin settings
    new_stdin_settings.c_lflag &= (~ICANON & ~ECHO); 

    // load new stdin settings
    tcsetattr(fileno(stdin), TCSANOW, &new_stdin_settings); 

    while (file_operator.is_open()) {
        fd_set set{};
        struct timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(fileno(stdin), &set);

        int read_readiness{
            select(fileno(stdin) + 1, &set, nullptr, nullptr, &tv)
        };
        if (read_readiness > 0) {
            char input_char{};
            read(fileno(stdin), &input_char, 1); // read from stdin

            handle_input(input_char);
        }
    }

    cout << endl;

    // load back original stdin settings
    tcsetattr(fileno(stdin), TCSANOW, &original_stdin_settings);
}

void CommandLine::handle_input(char input_char) {
    if (in_esc_mode) {
        handle_input_in_esc_mode(input_char);        
    }
    else {
        handle_input_in_regular_mode(input_char);
    }
}

void CommandLine::handle_input_in_esc_mode(char input_char) {
    in_esc_mode = false;

    ctrl_sequence += input_char;
    if (ctrl_sequence == "[") { // CSI
        // expecting more keys
        in_esc_mode = true;
    }
    else if (ctrl_sequence == "[A") { // arrow key up
        show_history_up();
    }
    else if (ctrl_sequence == "[B") { // arrow key down
        show_history_down();
    }
    else if (ctrl_sequence == "[C") { // arrow key right
        move_cursor_right();
    }
    else if (ctrl_sequence == "[D") { // arrow key left
        move_cursor_left();
    }
    else if (ctrl_sequence == "[3") { // precursor to DEL
        // expecting more keys
        in_esc_mode = true;
    }
    else if (ctrl_sequence == "[3~") { // DEL
        do_delete();
    }
}

void CommandLine::handle_input_in_regular_mode(char input_char) {
    switch (input_char) {
        case 27: // ESC
            in_esc_mode = true;
            ctrl_sequence = "";
            break;
        case 127: // Backspace
            do_backspace();
            break;
        case '\n':
            handle_newline();
            break;
        case 4: // ^D ... Ctrl + D
            exit();
            break;
        default:
            write_char(input_char);
            break;
    }
}

void CommandLine::show_history_up() {
    if (next_input_history_index < input_history.size()) {
        if (next_input_history_index == 0) {
            original_input = current_input;
        }

        update_input(input_history[next_input_history_index]);

        next_input_history_index++;
    }
}

void CommandLine::show_history_down() {
    if (next_input_history_index > 1) {
        next_input_history_index--;

        update_input(input_history[next_input_history_index - 1]);
    }
    else if (next_input_history_index == 1) {
        next_input_history_index = 0;

        update_input(original_input);
    }
}

void CommandLine::update_input(const string& input) {
    lock_guard console_lck{console_mtx};

    current_input = input;
    cursor_position = current_input.size();

    write_user_input();
}

void CommandLine::move_cursor_right() {
    if (cursor_position < current_input.size()) {
        lock_guard console_lck{console_mtx};

        cursor_position++;
        cout << "\33[C" << flush;
    }
}

void CommandLine::move_cursor_left() {
    if (cursor_position > 0) {
        lock_guard console_lck{console_mtx};

        cursor_position--;
        cout << "\33[D" << flush;
    }
}

void CommandLine::do_delete() {
    if (cursor_position < current_input.size()) {
        lock_guard console_lck{console_mtx};

        current_input.erase(cursor_position, 1);
        write_user_input(cursor_position);
    }
}

void CommandLine::do_backspace() {
    if (cursor_position > 0) {
        lock_guard console_lck{console_mtx};

        unsigned long char_to_remove_index{cursor_position - 1};
        cursor_position--;
        current_input.erase(char_to_remove_index, 1);
        write_user_input(char_to_remove_index);
    }
}

void CommandLine::handle_newline() {
    unique_lock console_lck{console_mtx};

    update_input_history(current_input);
    string command{current_input};
    current_input = "";
    cursor_position = 0;

    cout << "\n> " << flush;

    console_lck.unlock();

    execute(command);
}

void CommandLine::update_input_history(const string& input) {
    next_input_history_index = 0;

    if (input_history.size() == 0 || input != input_history[0]) {
        if (input_history.size() == max_input_history_size) {
            input_history.erase(input_history.end() - 1);
        }

        input_history.insert(input_history.begin(), input);
    }
}

void CommandLine::write_char(char output_char) {
    lock_guard console_lck{console_mtx};

    cursor_position++;

    if (cursor_position < current_input.size() - 1) {
        current_input.insert(
            current_input.begin() + cursor_position - 1, 
            output_char
        );
        write_user_input(cursor_position - 1);
    }
    else {
        current_input += output_char;
        cout << output_char << flush;
    }
}

void CommandLine::write_user_input(unsigned int start_index) {
    cout << "\r\33[" << prompt_length + start_index << "C"     // move to position of first char to change
         << "\33[K"                                            // clear all chars to right of cursor
         << current_input.substr(start_index)                  // new user input
         << "\r\33[" << prompt_length + cursor_position << "C" // move cursor to previous position
         << flush; 
}

void CommandLine::clear_line() {
    cout << "\33[2K\r" << flush;
}

void CommandLine::print_prompt_and_user_input() {
     cout << prompt << current_input                            // prompt and user input
          << "\r\33[" << prompt_length + cursor_position << "C" // move cursor to previous position
          << flush;
}
