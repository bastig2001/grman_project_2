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
#include <functional>
#include <mutex>

using namespace std;
using namespace placeholders;


CommandLine::CommandLine(
    Logger* logger, 
    Config& config
): logger{logger},
   config{config}
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


void CommandLine::operator()() {
    


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

void CommandLine::execute(const std::string&) {

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
        "signature\t" + 
        fmt::format(fg(fmt::color::olive), "size\t") +
        fmt::format(fg(fmt::color::cadet_blue), "last changed\t") +
        fmt::format(fg(fmt::color::burly_wood), "path\n\n")
    };

    for (auto file: 
            fs::get_files(fs::get_file_paths(config.sync.sync_hidden_files))
    ) {
        output += 
            file->signature() + "\t" +
            size_to_string(file->size()) + "\t" +
            fmt::format(
                fg(fmt::color::cadet_blue),
                time_to_string(
                    get_timepoint<std::chrono::system_clock>(file->timestamp())
            )) + "\t" +
            fmt::format(fg(fmt::color::burly_wood), file->name()) +
            "\n";
        
        delete file;
    }

    println(output);
}

void CommandLine::exit() {

}


void CommandLine::pre_output() {

}

void CommandLine::post_output() {

}
