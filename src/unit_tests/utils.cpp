#include "utils.h"
#include "unit_tests/doctest_utils.h"
#include "config.h"

#include <doctest.h>
#include <filesystem>
#include <optional>
#include <tuple>
#include <unordered_map>

using namespace std;


TEST_SUITE("utils") {
    TEST_CASE("optional to string") {
        SUBCASE("optional string to string") {
            vector<string> values{
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
            vector<int> values{0, -3, 1, 42, 999};
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

    TEST_CASE("vector to string") {
        SUBCASE("string vector to string") {
            vector<tuple<vector<string>, string, string, string>> value_tuples{
                {{"A", "B", "C"}, "A, B, C", "\n", "A\nB\nC"}, 
                {{"Test", "123"}, "Test, 123", "", "Test123"}, 
                {{"ABC"}, "ABC", "; ", "ABC"},
                {{}, "", ".", ""}
            };
            tuple<vector<string>, string, string, string> value_tuple;

            DOCTEST_VALUE_PARAMETERIZED_DATA(value_tuple, value_tuples);
            auto [elements, default_result, separator, separator_result]{value_tuple};

            CHECK(vector_to_string(elements) == default_result);
            CHECK(vector_to_string(elements, separator) == separator_result);
        }

        SUBCASE("path vector to string") {
            vector<tuple<vector<filesystem::path>, string, string, string>> value_tuples{
                {{"A", "B", "C"}, "A, B, C", "\n", "A\nB\nC"}, 
                {{"Test", "123"}, "Test, 123", "", "Test123"}, 
                {{"ABC"}, "ABC", "; ", "ABC"},
                {{}, "", ".", ""}
            };
            tuple<vector<filesystem::path>, string, string, string> value_tuple;

            DOCTEST_VALUE_PARAMETERIZED_DATA(value_tuple, value_tuples);
            auto [elements, default_result, separator, separator_result]{value_tuple};

            CHECK(vector_to_string(elements) == default_result);
            CHECK(vector_to_string(elements, separator) == separator_result);
        }
    }

    TEST_CASE("base 64 en- and decoding") {
        vector<tuple<string, string>> de_encoded_pairs{
            {"ABC", "QUJD"}, 
            {"Test123", "VGVzdDEyMw=="}, 
            {"Alles gut bei dir?", "QWxsZXMgZ3V0IGJlaSBkaXI/"},
            {"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark", "UG9seWZvbiB6d2l0c2NoZXJuZCBhw59lbiBNw6R4Y2hlbnMgVsO2Z2VsIFLDvGJlbiwgSm9naHVydCB1bmQgUXVhcms="}
        };
        tuple<string, string> de_encoded_pair;

        SUBCASE("encoding") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(de_encoded_pair, de_encoded_pairs);

            CHECK(to_base64(get<0>(de_encoded_pair)) == get<1>(de_encoded_pair));
        }

        SUBCASE("decoding") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(de_encoded_pair, de_encoded_pairs);

            CHECK(from_base64(get<1>(de_encoded_pair)) == get<0>(de_encoded_pair));
        }
    } 

    TEST_CASE("unordered_map contains") {
        unordered_map<string, string> map{
            {"A", ""}, {"B", ""}, {"C", ""}
        };

        vector<tuple<string, bool>> keys_if_contained{
            {"A", true}, {"B", true}, {"C", true}, {"D", false}, {"a", false}
        };
        tuple<string, bool> key_if_contained;

        DOCTEST_VALUE_PARAMETERIZED_DATA(key_if_contained, keys_if_contained);

        CHECK(contains(map, get<0>(key_if_contained)) == get<1>(key_if_contained));
    }  

    TEST_CASE("vector contains") {
        vector<string> values{"A", "B", "C"};

        vector<tuple<string, bool>> values_if_contained{
            {"A", true}, {"B", true}, {"C", true}, {"D", false}, {"a", false}
        };
        tuple<string, bool> value_if_contained;

        DOCTEST_VALUE_PARAMETERIZED_DATA(value_if_contained, values_if_contained);

        CHECK(contains(values, get<0>(value_if_contained)) == get<1>(value_if_contained));
    }   
} 
