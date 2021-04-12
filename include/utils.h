#pragma once

#include "messages/all.pb.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <istream>
#include <string>
#include <optional>
#include <type_traits>
#include <unordered_map>


std::string msg_to_base64(const Message&);
Message msg_from_base64(std::istream&);

std::string to_base64(const std::string&);
std::string from_base64(const std::string&);

// converts the given vector to a string with the specified separator
std::string vector_to_string(
    const std::vector<std::string>& elements, 
    const std::string& separator = ", "
);
std::string vector_to_string(
    const std::vector<std::filesystem::path>& elements, 
    const std::string& separator = ", "
);


template<typename T>
std::string optional_to_string(
    std::optional<T>& option, 
    const std::string& no_value = "None"
) {
    if (option.has_value()) {
        if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(option.value());
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return option.value();
        }
        else {
            return (std::string)option.value();
        }
    }
    else {
        return no_value;
    }
}


// checks if the given hashmap contains the sought-after key
template<typename K, typename V>
bool contains(
    const std::unordered_map<K, V>& map,
    const K& key 
) {
    return map.find(key) != map.end();
}

// checks if the given vector contains the sought-after value
template<typename T>
bool contains(
    const std::vector<T>& values,
    const T& value 
) {
    return std::find(values.begin(), values.end(), value) != values.end();
}


// returns the number of seconds between the given time point 
// and the beginning of the epoch (1st of January 1970)
template<typename T>
unsigned long get_timestamp(
    std::chrono::time_point<T> time_point
) {
    return 
        std::chrono::duration_cast<std::chrono::seconds>(
            time_point.time_since_epoch()
        ).count();
}

// returns for the given number of seconds after the beginning of the epoch
// the corresponding time point
template<typename T>
std::chrono::time_point<T> get_timepoint(
    unsigned long timestamp
) {
    return std::chrono::time_point<T>(std::chrono::seconds{timestamp});
}

// casts the given time point from the source clock 
// to a specified destination clock
template<
    typename DestTimePoint,
    typename SourceTimePoint,
    typename DestClock = typename DestTimePoint::clock,
    typename SourceClock = typename SourceTimePoint::clock
>
DestTimePoint cast_clock(const SourceTimePoint time_point) {
    const auto source_now = SourceClock::now();
    const auto dest_now = DestClock::now();

    return dest_now + (time_point - source_now);
}


// checks if the given future is ready
template<typename T>
bool is_ready(const std::future<T>& future) {
    return future.wait_for(std::chrono::milliseconds(10)) 
            == 
           std::future_status::ready;
}
