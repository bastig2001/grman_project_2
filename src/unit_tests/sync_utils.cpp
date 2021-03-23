#include "sync_utils.h"
#include "unit_tests/doctest_utils.h"

#include <doctest.h>
#include <tuple>

using namespace std;


TEST_SUITE("sync_utils") {
    TEST_CASE("MD5 hash") {
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

    TEST_CASE("rolling checksum") {
        vector<tuple<string, unsigned int>> msg_checksum_pairs{
            {"ABC", 25821382}, 
            {"Test123", 165872182}, 
            {"Alles gut bei dir?", 1017972303},
            {"Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark", 3512212438}
        };
        tuple<string, unsigned int> msg_checksum_pair;

        SUBCASE("checksum of whole value") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(msg_checksum_pair, msg_checksum_pairs);

            CHECK(
                get_rolling_checksum(
                    get<0>(msg_checksum_pair), 
                    0, 
                    get<0>(msg_checksum_pair).length()
                ) 
                == 
                get<1>(msg_checksum_pair)
            );
        }
        
        SUBCASE("rolling incremented checksum") {
            DOCTEST_VALUE_PARAMETERIZED_DATA(msg_checksum_pair, msg_checksum_pairs);
            string msg{get<0>(msg_checksum_pair)};

            auto checksums{get_rolling_checksums(msg, 0, 3)};
            for (unsigned int i{0}; i < checksums.size(); i++) {
                CHECK(checksums[i] == get_rolling_checksum(msg, i, 3));
            }
        }
    }
}
