#include "config.h"

#include <CLI11.hpp>

using namespace std;


variant<ExitCode, Config> configure(int argc, char* argv[]) {
    CLI::App app("File Synchronisation Client");

    CLI11_PARSE(app, argc, argv);

    return Config();
}
