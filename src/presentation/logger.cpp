#include "exit_code.h"
#include "presentation/logger.h"

#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

using namespace std;

Logger* logger{new NoLogger};

BasicLogger* get_basic_console_logger(spdlog::level::level_enum, bool); 
BasicLogger* get_basic_file_logger(
    spdlog::level::level_enum, bool, const string&, size_t, size_t
); 
void write_log_start(Logger*);
string get_pattern(bool);


Logger* get_logger(const LogConfig& config) {
    auto console_logger{
        get_basic_console_logger(
            config.log_to_console 
                ? config.level_console 
                : spdlog::level::err, 
            config.log_date
        )};

    if (config.file.has_value()) {
        return new ChainLogger(
            console_logger,
            get_basic_file_logger(
                config.level_console, 
                config.log_date, 
                config.file.value(),
                config.max_file_size,
                config.number_of_files
            )
        );
    }
    else {
        return console_logger;
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
    const string& file,
    size_t max_file_size,
    size_t number_of_files
) {
    try {
        auto logger{new BasicLogger(
            spdlog::rotating_logger_mt(
                "file", file, max_file_size, number_of_files
            ),
            true
        )};

        write_log_start(logger);

        logger->set_level(level);
        logger->set_pattern(get_pattern(log_date));

        return logger;
    }
    catch (const spdlog::spdlog_ex& err) {
        cerr << ("Writing to the log file failed:\n") << err.what() << endl;
        exit(LogFileError);
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
