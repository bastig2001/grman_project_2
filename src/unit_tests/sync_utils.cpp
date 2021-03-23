#include "sync_utils.h"
#include "unit_tests/doctest_utils.h"

#include <doctest.h>
#include <sstream>
#include <tuple>

using namespace std;


TEST_SUITE("sync_utils") {
    TEST_CASE("strong signature") {
        vector<tuple<string, string>> msg_signature_pairs{
            {"ABC", "902fbdd2b1df0c4f70b4a5d23525e932"}, 
            {"Test123", "68eacb97d86f0c4621fa2b0e17cabd8c"}, 
            {"Alles gut bei dir?", "2ed21e45fac7d66719df2e58080b0508"},
            {"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark", "823b476aeb6e7fc02afec186079ac107"}
        };
        tuple<string, string> msg_signature_pair;

        SUBCASE("string") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(msg_signature_pair, msg_signature_pairs);

            CHECK(
                get_strong_signature(get<0>(msg_signature_pair)) 
                == 
                get<1>(msg_signature_pair)
            );
        }
        SUBCASE("istream") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(msg_signature_pair, msg_signature_pairs);
            istringstream msg_stream{get<0>(msg_signature_pair)};
            CHECK(
                get_strong_signature(msg_stream) == get<1>(msg_signature_pair)
            );
        }        
    }

    TEST_CASE("weak signature") {
        vector<tuple<string, unsigned int>> msg_signature_pairs{
            {"ABC", 25821382}, 
            {"Test123", 165872182}, 
            {"Alles gut bei dir?", 1017972303},
            {"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark", 3512212438}
        };
        tuple<string, unsigned int> msg_signature_pair;

        SUBCASE("checksum of whole value") {
            SUBCASE("string") {
                DOCTEST_VALUE_PARAMETERIZED_DATA(msg_signature_pair, msg_signature_pairs);

                CHECK(
                    get_weak_signature(
                        get<0>(msg_signature_pair), 
                        0, 
                        get<0>(msg_signature_pair).length()
                    ) 
                    == 
                    get<1>(msg_signature_pair)
                );
            }
            SUBCASE("istream") {
                DOCTEST_VALUE_PARAMETERIZED_DATA(msg_signature_pair, msg_signature_pairs);
                istringstream msg_stream{get<0>(msg_signature_pair)};

                CHECK(
                    get_weak_signature(
                        msg_stream, 
                        0, 
                        get<0>(msg_signature_pair).length()
                    ) 
                    == 
                    get<1>(msg_signature_pair)
                );
            }
        }
        
        SUBCASE("rolling incremented checksum") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(msg_signature_pair, msg_signature_pairs);
            string msg{get<0>(msg_signature_pair)};

            auto signatures{get_weak_signatures(msg, 0, 3)};
            for (unsigned int i{0}; i < signatures.size(); i++) {
                CHECK(signatures[i] == get_weak_signature(msg, i, 3));
            }
        }
    }
}
