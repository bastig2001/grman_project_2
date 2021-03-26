#pragma once

#include "messages/all.pb.h"

#include <filesystem>
#include <istream>
#include <string>
#include <optional>
#include <type_traits>


std::string msg_to_base64(const Message&);
Message msg_from_base64(std::istream&);

std::string to_base64(const std::string&);
std::string from_base64(const std::string&);

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
