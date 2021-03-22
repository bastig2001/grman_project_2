#include "utils.h"
#include "unit_tests/doctest_utils.h"
#include "config.h"

#include <doctest.h>
#include <optional>
#include <tuple>

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

    TEST_CASE("get MD5 hash") {
        vector<tuple<string, string>> msg_digest_pairs{
            {"ABC", "902fbdd2b1df0c4f70b4a5d23525e932"}, 
            {"Test123", "68eacb97d86f0c4621fa2b0e17cabd8c"}, 
            {"Alles gut bei dir?", "2ed21e45fac7d66719df2e58080b0508"},
            {"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark", "823b476aeb6e7fc02afec186079ac107"}
        };
        tuple<string, string> msg_digest_pair;
        DOCTEST_VALUE_PARAMETERIZED_DATA(msg_digest_pair, msg_digest_pairs);

        CHECK(get_md5_hash(get<0>(msg_digest_pair)) == get<1>(msg_digest_pair));
    }
} 
