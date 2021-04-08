#include "presentation/format_utils.h"

#include <fmt/core.h>
#include <fmt/color.h>
#include <cmath>

using namespace std;
using namespace fmt;


string size_to_string(size_t size) {
    if (size >= pow(2, 40)) {
        return format(fg(color::olive), "{:.2}", size / pow(2, 40))
             + format(fg(color::sea_green), "TiB");
    }
    else if (size >= pow(2, 30)) {
        return format(fg(color::olive), "{:.2}", size / pow(2, 30))
             + format(fg(color::sea_green), "GiB");
    }
    else if (size >= pow(2, 20)) {
        return format(fg(color::olive), "{:.2}", size / pow(2, 20))
             + format(fg(color::sea_green), "MiB");
    }
    else if (size >= pow(2, 10)) {
        return format(fg(color::olive), "{:.2}", size / pow(2, 10))
             + format(fg(color::sea_green), "KiB");
    }
    else {
        return format(fg(color::olive), "{}", size)
             + format(fg(color::sea_green), "B");
    }
}
