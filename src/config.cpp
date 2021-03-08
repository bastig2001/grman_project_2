#include "config.h"

#include <CLI11.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/address.hpp>

using namespace std;

string is_ip_address(const std::string& address);


variant<ExitCode, Config> configure(int argc, char* argv[]) {
    CLI::App app("File Synchronisation Client");

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

    LogConfig log{};
    app.add_flag(
        "-l, --log-to-console",
        log.log_to_console,
        "Enables logging to console\n"
            "  Default logging level is INFO"
    )->envname("SYNC_LOG_CONSOLE");
    string log_file{""};
    auto log_file_option = 
        app.add_option(
            "-f, --log-file",
            log_file,
            "Enables logging to specified file\n"
                "  Default logging level is INFO"
        )->envname("SYNC_LOG_FILE");
    auto log_level_option = 
        app.add_option(
            "--log-level, --log-level-console",
            log.level_console,
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
            log.level_file,
            "Sets the visible logging level for the log file\n"
                "  By default it takes the logging level specified with --log-level"
        )->envname("SYNC_LOG_LEVEL_FILE");
    app.add_option(
        "--log-size",
        log.max_file_size,
        "Maximum size of a log file in KB\n"
            "  Default is 5 MB"
    )->envname("SYNC_LOG_SIZE");
    app.add_option(
        "--log-file-number",
        log.number_of_files,
        "Number of log files between which the logger rotates\n"
            " Default is 2"
    )->envname("SYNC_LOG_FILE_NUMBER");
    app.add_flag(
        "--log-date",
        log.log_date,
        "Logs the date additionally to the time, when logging to a file"
    )->envname("SYNC_LOG_DATE");
    app.add_flag(
        "--log-config",
        log.log_config,
        "Log the used config as a DEBUG message"
    )->envname("SYNC_LOG_CONFIG");

    CLI11_PARSE(app, argc, argv);

    // if any of these three have been set, the program shall act as server
    serve = serve || *bind_address_option || *bind_port_option;

    log.max_file_size *= 1024; // convert from KB to B
    log.file = *log_file_option ? optional{log_file} : nullopt;
    if (!*log_level_file_option && *log_level_option) {
        // set log.level_file to log.level_console, 
        //   when the latter was specified but not the first 
        log.level_file = log.level_console;
    }

    return Config{
        *server_address_option ? optional{server} : nullopt,
        serve ? optional{act_as_server} : nullopt,
        log
    };
}

string is_ip_address(const std::string& address) {
    asio::error_code err{};
    asio::ip::make_address(address, err);

    return err ? err.message() : "";
}


LogConfig::operator std::string() {
    std::ostringstream output{};

    output 
        << std::boolalpha
        << " {\n"
        << "  \"log to console\":   " << log_to_console               << ";\n"
        << "  \"file\":             \"" << optional_to_string(file) << "\";\n"
        << "  \"level on console\": " << level_console                << ";\n"
        << "  \"level in file\":    " << level_file                   << ";\n"
        << "  \"max file size\":    " << max_file_size / 1024         << ";\n"
        << "  \"number of files\":  " << number_of_files              << ";\n"
        << "  \"log date\":         " << log_date                     << ";\n"
        << "  \"log config\":       " << log_config                    << "\n"
        << " }";

    return output.str();
}
