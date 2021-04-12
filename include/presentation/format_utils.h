#pragma once

#include "messages/basic.h"
#include "type/definitions.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>


// all functions which can output color do it only when this variable is set true
extern bool use_color;

// returns the header line for the command line file list output
std::string get_file_list_header();

// returns the given file formatted for the command line file list output
std::string format_file(const msg::File&);

// returns the formatted size with the corresponding size unit (B, KiB, ...)
std::string format_size(size_t);

// returns the file name of the given file colored
std::string colored(const msg::File&);

// returns the given file name colored
std::string colored(const FileName&);

// converts the given time point to string
template<typename T>
std::string time_to_string(std::chrono::time_point<T> time_point) {
    std::time_t time{T::to_time_t(time_point)};
    
    std::ostringstream result{};
    result << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    return result.str();
}
