#include "presentation/command_line.h"

#include <spdlog/spdlog.h>
#include <mutex>

using namespace std;


CommandLine::CommandLine(Logger* logger): logger{logger} {
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


void CommandLine::pre_output() {

}

void CommandLine::post_output() {

}
