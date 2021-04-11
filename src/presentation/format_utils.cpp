#include "presentation/format_utils.h"
#include "utils.h"

#include <fmt/core.h>
#include <fmt/color.h>
#include <cmath>
#include <filesystem>
#include <utility>

using namespace std;
using namespace fmt;

bool use_color{};

pair<string, string> get_size_and_unit(size_t);


string get_file_list_header() {
    if (use_color) {
        return
            "signature" + string(32 + 2 - 9, ' ') +  
            fmt::format(fg(fmt::color::sea_green), "size      ") +
            fmt::format(fg(fmt::color::cadet_blue), "last changed         ") +
            fmt::format(fg(fmt::color::burly_wood), "path\n\n");
    }
    else {
        return
            "signature" + string(32 + 2 - 9, ' ') +  
            "size     \t" +
            "last changed         " +
            "path\n\n";
    }
}

string format_file(const msg::File& file) {
    auto time{
        time_to_string(
            cast_clock<chrono::time_point<chrono::system_clock>>(
                get_timepoint<filesystem::file_time_type::clock>(
                    file.timestamp
    )))};

    return 
        file.signature + "  " +
        format_size(file.size) + "  " +
        (use_color
            ? fmt::format(fg(fmt::color::cadet_blue), time)
            : time
        ) + "  " +
        colored(file);
}

string format_size(size_t size) {
    auto size_and_unit{get_size_and_unit(size)};

    if (use_color) {
        return format(fg(color::sea_green), size_and_unit.first) 
             + format(fg(color::green), size_and_unit.second);
    }
    else {
        return size_and_unit.first + size_and_unit.second;
    }
}

pair<string, string> get_size_and_unit(size_t size) {
    if (size >= pow(2, 40)) {
        return {
            format("{:>{}}", format("{:2.1f}", size / pow(2, 40)), 5), 
            "TiB"
        };
    }
    else if (size >= pow(2, 30)) {
        return {
            format("{:>{}}", format("{:2.1f}", size / pow(2, 30)), 5), 
            "GiB"
        };
    }
    else if (size >= pow(2, 20)) {
        return {
            format("{:>{}}", format("{:2.1f}", size / pow(2, 20)), 5), 
            "MiB"
        };
    }
    else if (size >= pow(2, 10)) {
        return {
            format("{:>{}}", format("{:2.1f}", size / pow(2, 10)), 5), 
            "KiB"
        };
    }
    else {
        return {
            format("{:>{}}", format("{}  ", size), 5), 
            "  B"
        };
    }
}

string colored(const msg::File& file) {
    return colored(file.name);
}

string colored(const FileName& file) {
    if (use_color) {
        return fmt::format(fg(fmt::color::burly_wood), file);
    }
    else {
        return file;
    }
}
