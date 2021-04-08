#pragma once

#include <json.hpp>
#include <optional>


using nlohmann::json;


template<typename T>
json optional_to_json(
    const std::optional<T>& option, 
    const json& no_value = json({})
) {
    if (option.has_value()) {
        return json{option.value()}[0];
    }
    else {
        return no_value;
    }
}

template<typename T>
std::optional<T> json_to_optional(
    const json& value, 
    const json& no_value = json({})
) {
    if (value != no_value) {
        return value.get<T>();
    }
    else {
        return std::nullopt;
    }
}


// functions for implicit (de)serialization between json an optional:

template<typename T>
void to_json(json& j, const std::optional<T>& option) {
    j = optional_to_json<T>(option);
}

template<typename T>
void from_json(const json& j, std::optional<T>& option) {
    option = json_to_optional<T>(j);
}
