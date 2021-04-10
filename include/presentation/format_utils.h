#pragma once

#include "messages/basic.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>


extern bool use_color;

std::string get_file_list_header();

std::string format_file(const msg::File&);

std::string format_size(size_t);

std::string colored(const msg::File&);


template<typename T>
std::string time_to_string(std::chrono::time_point<T> time_point) {
    std::time_t time{T::to_time_t(time_point)};
    
    std::ostringstream result{};
    result << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    return result.str();
}
