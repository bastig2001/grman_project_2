#include "utils.h"
#include "unit_tests/doctest_utils.h"
#include "config.h"

#include <doctest.h>
#include <optional>

using namespace std;


TEST_SUITE("utils") {
    TEST_CASE("optional to string") {
        SUBCASE("optional string to string") {
            vector<string> values = {
                "ABC", 
                "Test123", 
                "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark"
            };
            string value;
            optional<string> opt_val;
        
            SUBCASE("optional has value") {
                DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);
                opt_val = value;

                CHECK(optional_to_string(opt_val) == value);
            }

            SUBCASE("optional has no value") {
                opt_val = nullopt;

                CHECK(optional_to_string(opt_val) == "None");
            }

            SUBCASE("optional has no value and an alternative is defined") {
                opt_val = nullopt;
                DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);

                CHECK(optional_to_string(opt_val, value) == value);
            }
        }

        SUBCASE("optional int to string") {
            vector<int> values = {0, -3, 1, 42, 999};
            int value;
            optional<int> opt_val;
        
            SUBCASE("optional has value") {
                DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);
                opt_val = value;

                CHECK(optional_to_string(opt_val) == to_string(value));
            }

            SUBCASE("optional has no value") {
                opt_val = nullopt;

                CHECK(optional_to_string(opt_val) == "None");
            }

            SUBCASE("optional has no value and an alternative is defined") {
                opt_val = nullopt;
                DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);

                CHECK(optional_to_string(opt_val, to_string(value)) == to_string(value));
            }
        }

        SUBCASE("optional ServerData object to string") {
            vector<ServerData> values = {
                ServerData{},
                ServerData{"localhost", 9999},
                ServerData{"1.2.3.4", 1234}
            };
            ServerData value;
            optional<ServerData> opt_val;
        
            SUBCASE("optional has value") {
                DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);
                opt_val = value;

                CHECK(optional_to_string(opt_val) == (string)value);
            }

            SUBCASE("optional has no value") {
                opt_val = nullopt;

                CHECK(optional_to_string(opt_val) == "None");
            }

            SUBCASE("optional has no value and an alternative is defined") {
                opt_val = nullopt;
                DOCTEST_VALUE_PARAMETERIZED_DATA(value, values);

                CHECK(optional_to_string(opt_val, (string)value) == (string)value);
            }
        }
    }
}
