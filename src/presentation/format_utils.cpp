#include "presentation/format_utils.h"

#include <fmt/core.h>
#include <fmt/color.h>
#include <cmath>

using namespace std;
using namespace fmt;


string size_to_string(size_t size) {
    if (size >= pow(2, 40)) {
        return 
            format(
                fg(color::sea_green), 
                "{:>{}}",
                format("{:2.1f}", size / pow(2, 40)), 
                5
            ) + 
            format(fg(color::green), "TiB");
    }
    else if (size >= pow(2, 30)) {
        return 
            format(
                fg(color::sea_green), 
                "{:>{}}",
                format("{:2.1f}", size / pow(2, 30)), 
                5
            ) + 
            format(fg(color::green), "GiB");
    }
    else if (size >= pow(2, 20)) {
        return 
            format(
                fg(color::sea_green), 
                "{:>{}}",
                format("{:2.1f}", size / pow(2, 20)), 
                5
            ) + 
            format(fg(color::green), "MiB");
    }
    else if (size >= pow(2, 10)) {
        return 
            format(
                fg(color::sea_green), 
                "{:>{}}",
                format("{:2.1f}", size / pow(2, 10)), 
                5
            ) + 
            format(fg(color::green), "KiB");
    }
    else {
        return 
            format(
                fg(color::sea_green), 
                "{:>{}}",
                format("{}", size), 
                3
            ) + 
            format(fg(color::green), "    B");
    }
}
