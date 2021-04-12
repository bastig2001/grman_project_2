#include "json_utils.h"
#include "config.h"
#include "unit_tests/doctest_utils.h"

#include <json.hpp>
#include <doctest.h>
#include <optional>
#include <utility>
#include <vector>

using namespace std;


TEST_SUITE("json_utils") {
    TEST_CASE("optional to json") {
        vector<ServerData> values{
            ServerData{},
            ServerData{"localhost", 9999},
            ServerData{"1.2.3.4", 1234}
        };
        ServerData value;
        optional<ServerData> opt_val;
    
        SUBCASE("optional has value") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);
            opt_val = value;

            CHECK(optional_to_json(opt_val) == json(value));
        }

        SUBCASE("optional has no value") {
            opt_val = nullopt;

            CHECK(optional_to_json(opt_val) == json({}));
        }
    }
    TEST_CASE("implicit optional to json") {
        vector<ServerData> values{
            ServerData{},
            ServerData{"localhost", 9999},
            ServerData{"1.2.3.4", 1234}
        };
        ServerData value;
        optional<ServerData> opt_val;
    
        SUBCASE("optional has value") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);
            opt_val = value;

            CHECK(json(opt_val) == json(value));
        }

        SUBCASE("optional has no value") {
            opt_val = nullopt;

            CHECK(json(opt_val) == json({}));
        }
    }

    TEST_CASE("json to optional") {
        vector<pair<json, ServerData>> values{
            {R"({"address":"0.0.0.0","port":9876})"_json, 
                ServerData{}},
            {R"({"address":"localhost","port":9999})"_json, 
                ServerData{"localhost", 9999}},
            {R"({"address":"1.2.3.4","port":1234})"_json, 
                ServerData{"1.2.3.4", 1234}}
        };
        pair<json, ServerData> value_pair;

        SUBCASE("there is a value") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(value_pair, values);
            auto [j, data]{value_pair};
            auto json_data{json_to_optional<ServerData>(j)};

            REQUIRE(json_data.has_value());
            CHECK(json_data.value().address == data.address);
            CHECK(json_data.value().port == data.port);
        }

        SUBCASE("there is no value") {
            CHECK(json_to_optional<optional<ServerData>>(json({})) == nullopt);
        }
    }
    TEST_CASE("implicit json to optional") {
        vector<pair<json, ServerData>> values{
            {R"({"address":"0.0.0.0","port":9876})"_json, 
                ServerData{}},
            {R"({"address":"localhost","port":9999})"_json, 
                ServerData{"localhost", 9999}},
            {R"({"address":"1.2.3.4","port":1234})"_json, 
                ServerData{"1.2.3.4", 1234}}
        };
        pair<json, ServerData> value_pair;

        SUBCASE("there is a value") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(value_pair, values);
            auto [j, data]{value_pair};
            auto json_data{j.get<optional<ServerData>>()};

            REQUIRE(json_data.has_value());
            CHECK(json_data.value().address == data.address);
            CHECK(json_data.value().port == data.port);
        }

        SUBCASE("there is no value") {
            CHECK(json({}).get<optional<ServerData>>() == nullopt);
        }
    }
}
