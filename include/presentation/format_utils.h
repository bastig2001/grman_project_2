#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>


std::string size_to_string(size_t);


template<typename T>
std::string time_to_string(std::chrono::time_point<T> time_point) {
    std::time_t time{T::to_time_t(time_point)};
    
    std::ostringstream result{};
    result << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    return result.str();
}
