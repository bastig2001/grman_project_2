#pragma once

#include <string>
#include <optional>
#include <type_traits>


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

std::string encode_base64(const std::string&);

std::string decode_base64(const std::string&);
