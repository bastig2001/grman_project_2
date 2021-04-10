#include "config.h"
#include "presentation/logger_config.h"

#include <CLI11.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/address.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <variant>

using namespace std;

optional<Config> read_config(const std::string&);
variant<int, Config> override_config(Config&&, int, char*[]);
string is_ip_address(const std::string& address);


variant<int, Config> configure(int argc, char* argv[]) {
    CLI::App app("File Synchronisation Client");

    string config_file{""};
    app.add_option(
        "-c, --config",
        config_file,
        "JSON config file from which to load the configuration\n"
            "  Config values can be overriden with the CLI"
    )
    ->envname("SYNC_CONFIG")
    ->check(CLI::ExistingFile);

    ServerData server{};
    auto server_address_option = 
        app.add_option(
            "-a, --server-address",
            server.address,
            "The address of the server to which to connect for syncing"
        )->envname("SYNC_SERVER_ADDRESS");
    app.add_option(
        "-p, --server-port",
        server.port,
        "The port of the server to which to connect for syncing\n"
            "  Defaults to 9876"
    )->envname("SYNC_SERVER_PORT");

    bool serve{false};
    app.add_flag(
        "-s, --serve",
        serve,
        "Act as server to other clients for syncing\n"
            "  For binding address and port see --bind-address and --bind-port"
    )->envname("SYNC_SERVE");

    ServerData act_as_server{};
    auto bind_address_option = 
        app.add_option(
            "--bind-address",
            act_as_server.address,
            "The address to which to bind as server\n"
                "  Defaults to 0.0.0.0 (listen to all)\n"
                "  Enables the flag --serve"
        )->envname("SYNC_BIND_ADDRESS")
         ->check(is_ip_address);
    auto bind_port_option = 
        app.add_option(
            "--bind-port",
            act_as_server.port,
            "The port to which to bind as server\n"
                "  Defaults to 9876\n"
                "  Enables the flag --serve"
        )->envname("SYNC_BIND_PORT");

    SyncConfig sync{};
    app.add_flag(
        "--hidden",
        sync.sync_hidden_files,
        "Sync also hidden files"
    )->envname("SYNC_HIDDEN");

    LoggerConfig logger{};
    app.add_flag(
        "-l, --log-to-console",
        logger.log_to_console,
        "Enables logging to console\n"
            "  Default logging level is INFO"
    )->envname("SYNC_LOG_CONSOLE");
    app.add_option(
        "-f, --log-file",
        logger.file,
        "Enables logging to specified file\n"
            "  Default logging level is INFO"
    )->envname("SYNC_LOG_FILE");
    auto log_level_option = 
        app.add_option(
            "--log-level, --log-level-console",
            logger.level_console,
            "Sets the visible logging level\n"
                "  0 ... TRACE\n"
                "  1 ... DEBUG\n"
                "  2 ... INFO\n"
                "  3 ... WARN\n"
                "  4 ... ERROR\n"
                "  5 ... CRITICAL"
        )->envname("SYNC_LOG_LEVEL");
    auto log_level_file_option =
        app.add_option(
            "--log-level-file",
            logger.level_file,
            "Sets the visible logging level for the log file\n"
                "  By default it takes the logging level specified with --log-level"
        )->envname("SYNC_LOG_LEVEL_FILE");
    app.add_option(
        "--log-size",
        logger.max_file_size,
        "Maximum size of a log file in KiB\n"
            "  Default is 5 MiB"
    )->envname("SYNC_LOG_SIZE");
    app.add_option(
        "--log-file-number",
        logger.number_of_files,
        "Number of log files between which the logger rotates\n"
            " Default is 2"
    )->envname("SYNC_LOG_FILE_NUMBER");
    app.add_flag(
        "--log-date",
        logger.log_date,
        "Logs the date additionally to the time"
    )->envname("SYNC_LOG_DATE");
    app.add_flag(
        "--log-config",
        logger.log_config,
        "Log the used config as a DEBUG message"
    )->envname("SYNC_LOG_CONFIG");

    bool no_color{false};
    app.add_flag(
        "--no-color",
        no_color,
        "Disables color output to console"
    )->envname("SYNC_NO_COLOR");

    CLI11_PARSE(app, argc, argv);

    if (config_file == "") {
        // if any of these three have been set, the program shall act as server
        serve = serve || *bind_address_option || *bind_port_option;

        logger.max_file_size *= 1024; // convert from KB to B

        if (!*log_level_file_option && *log_level_option) {
            // set log.level_file to log.level_console, 
            //   when the latter was specified but not the first 
            logger.level_file = logger.level_console;
        }

        return Config{
            *server_address_option ? optional{server} : nullopt,
            serve ? optional{act_as_server} : nullopt,
            sync,
            logger,
            no_color
        };
    }
    else {
        auto config{read_config(config_file)};

        if (config.has_value()) {
            return override_config(move(config.value()), argc, argv);
        }
        else {
            return 109; // CLI Error
        }
    }
    
}

optional<Config> read_config(const std::string& file_name) {
    try {
        ifstream file{file_name};
        json j;
        file >> j;

        Config config{j.get<Config>()};

        if (config.act_as_server.has_value()) {
            // bind IP address needs to be checked

            string ip_address_err{
                is_ip_address(config.act_as_server.value().address)
            }; 
            
            if (ip_address_err == "") {
                return config;
            }
            else {
                cerr << "Address of \"act as server\" in config file"
                        "is not an IP address: " << ip_address_err << endl;

                return nullopt;
            }
        }
        else {
            return config;
        }
    }
    catch (const exception& err) {
        cerr << "The config file couldn't be parsed: " << err.what() << endl;

        return nullopt;
    }
}

variant<int, Config> override_config(Config&& config, int argc, char* argv[]) {
    CLI::App app("File Synchronisation Client");

    string config_file{""};
    app.add_option(
        "-c, --config",
        config_file
    );

    ServerData server{
        config.server.has_value()
        ? ServerData{
            config.server.value().address, 
            config.server.value().port
          }
        : ServerData{}
    };
    auto server_address_option = 
        app.add_option(
            "-a, --server-address",
            server.address
        );
    app.add_option(
        "-p, --server-port",
        server.port
    );

    bool serve{config.act_as_server.has_value()};
    app.add_flag(
        "-s, --serve",
        serve
    );

    ServerData act_as_server{
        config.act_as_server.has_value()
        ? ServerData{
            config.act_as_server.value().address, 
            config.act_as_server.value().port
          }
        : ServerData{}
    };
    auto bind_address_option = 
        app.add_option(
            "--bind-address",
            act_as_server.address
        );
    auto bind_port_option = 
        app.add_option(
            "--bind-port",
            act_as_server.port
        );

    SyncConfig sync{move(config.sync)};
    app.add_flag(
        "--hidden",
        sync.sync_hidden_files
    );

    LoggerConfig logger{move(config.logger)};
    app.add_flag(
        "-l, --log-to-console",
        logger.log_to_console
    );
    app.add_option(
        "-f, --log-file",
        logger.file
    );
    auto log_level_option = 
        app.add_option(
            "--log-level, --log-level-console",
            logger.level_console
        );
    auto log_level_file_option =
        app.add_option(
            "--log-level-file",
            logger.level_file
        );
    app.add_option(
        "--log-size",
        logger.max_file_size
    );
    app.add_option(
        "--log-file-number",
        logger.number_of_files
    );
    app.add_flag(
        "--log-date",
        logger.log_date
    );
    app.add_flag(
        "--log-config",
        logger.log_config
    );

    bool no_color{config.no_color};
    app.add_flag(
        "--no-color",
        no_color
    );

    CLI11_PARSE(app, argc, argv);

    // if any of these three have been set, the program shall act as server
    serve = serve || *bind_address_option || *bind_port_option;

    logger.max_file_size *= 1024; // convert from KB to B

    if (!*log_level_file_option && *log_level_option) {
        // set log.level_file to log.level_console, 
        //   when the latter was specified but not the first 
        logger.level_file = logger.level_console;
    }

    return Config{
        *server_address_option || config.server.has_value() 
            ? optional{server} 
            : nullopt,
        serve ? optional{act_as_server} : nullopt,
        sync,
        logger,
        no_color
    };
}

string is_ip_address(const std::string& address) {
    asio::error_code err{};
    asio::ip::make_address(address, err);

    return err ? err.message() : "";
}
