#include "presentation/logger.h"
#include "spdlog/common.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

using namespace std;

Logger* logger{new NoLogger};

BasicLogger* get_basic_console_logger(spdlog::level::level_enum, bool); 
BasicLogger* get_basic_file_logger(spdlog::level::level_enum, bool, const string&); 
void write_log_start(Logger*);
string get_pattern(bool);


Logger* get_logger(const LogConfig& config) {
    if (config.log_to_console || config.log_file.has_value()) {
        if (config.log_to_console && config.log_file.has_value()) {
            return new ChainLogger(
                get_basic_console_logger(
                    config.log_level_console, 
                    config.log_date
                ),
                get_basic_file_logger(
                    config.log_level_console, 
                    config.log_date, 
                    config.log_file.value()
                )
            );
        }
        else if (config.log_to_console) {
            return get_basic_console_logger(
                config.log_level_console, 
                config.log_date
            );
        }
        else {
            return get_basic_file_logger(
                config.log_level_console, 
                config.log_date, 
                config.log_file.value()
            );
        }
    }
    else {
        return new NoLogger;
    }
}

BasicLogger* get_basic_console_logger(
    spdlog::level::level_enum level, 
    bool log_date
) {
    auto logger{new BasicLogger(
        spdlog::stdout_color_mt("console"), 
        false
    )};
    logger->set_level(level);
    logger->set_pattern(get_pattern(log_date));

    return logger;
}

BasicLogger* get_basic_file_logger(
    spdlog::level::level_enum level, 
    bool log_date, 
    const string& file
) {
    try {
        auto logger{new BasicLogger(
            spdlog::rotating_logger_mt("file", file, 1048576, 3),
            true
        )};

        write_log_start(logger);

        logger->set_level(level);
        logger->set_pattern(get_pattern(log_date));

        return logger;
    }
    catch (const spdlog::spdlog_ex& err) {
        cerr << ("Writing to the log file failed:\n") << err.what() << endl;
        exit(65);
    }
}

void write_log_start(Logger* logger) {
    logger->set_pattern("%v");
    logger->set_level(spdlog::level::info);

    logger->info("=========================================================");
    logger->set_pattern("  This is a new Log starting at %Y-%m-%d %T.%e");
    logger->info("");
    logger->set_pattern("%v");
    logger->info("=========================================================");
}

string get_pattern(bool log_date) {
    string date_pattern{
        log_date
        ? "%Y-%m-%d "
        : ""
    };
    return "[" + date_pattern + "%T.%e] [%^%l%$] %v";
}
