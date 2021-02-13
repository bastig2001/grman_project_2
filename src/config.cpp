#include "config.h"

#include <CLI11.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/address.hpp>
#include <iostream>

using namespace std;

string is_ip_address(const std::string& address);


variant<ExitCode, Config> configure(int argc, char* argv[]) {
    CLI::App app("File Synchronisation Client");

    Server server{};
    app.add_option(
        "-a, --server-address",
        server.address,
        "The address of the server to which to connect for syncing"
    )->envname("SYNC_SERVER_ADDRESS")
     ->check(is_ip_address);
    app.add_option(
        "-p, --server-port",
        server.port,
        "The port of the server to which to connect for syncing\n"
            "Defaults to 9876"
    )->envname("SYNC_SERVER_PORT");

    bool serve{false};
    app.add_flag(
        "-s, --serve",
        serve,
        "Act as server to other clients for syncing\n"
            "For binding address and port see --bind-address and --bind-port"
    )->envname("SYNC_SERVE");

    Server act_as_server{};
    auto bind_address_option = 
        app.add_option(
            "--bind-address",
            act_as_server.address,
            "The address to which to bind as server\n"
                "Defaults to 0.0.0.0 (listen to all)\n"
                "Enables the flag --serve"
        )->envname("SYNC_BIND_ADDRESS")
         ->check(is_ip_address);
    auto bind_port_option = 
        app.add_option(
            "--bind-port",
            act_as_server.port,
            "The port to which to bind as server\n"
                "Defaults to 9876\n"
                "Enables the flag --serve"
        )->envname("SYNC_BIND_PORT");

    CLI11_PARSE(app, argc, argv);

    // if any of these three have been set, the program shall act as server
    serve = serve || *bind_address_option || *bind_port_option;

    return Config{
        server.address != "0.0.0.0" ? optional{server} : nullopt,
        serve ? optional{act_as_server} : nullopt
    };
}

string is_ip_address(const std::string& address) {
    asio::error_code err{};
    asio::ip::address::from_string(address, err);

    return err ? err.message() : "";
}
