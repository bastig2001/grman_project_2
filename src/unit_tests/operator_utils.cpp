#include "file_operator/operator_utils.h"
#include "unit_tests/doctest_utils.h"

#include <doctest.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;

using TestData = 
    tuple<
        vector<unsigned long>, // offsets
        size_t, // file size
        string, // file name
        unsigned int, // block size
        vector<pair<
            unsigned long, // offset
            unsigned int // size
        >> // expected resulting block values
    >;

TEST_SUITE("operator utils") {
    TEST_CASE("get blocks between") {
        vector<TestData> all_values{
            {{20, 50, 60, 70}, 90, "ABC", 10, {{0, 20}, {30, 20}, {80, 10}}},
            {{0, 5, 12, 24}, 30, "xyz", 5, {{10, 2}, {17, 7}, {29, 1}}},
            {{70, 50, 0, 60}, 90, "test", 10, {{10, 40}, {80, 10}}}
        };
        TestData values;

        DOCTEST_VALUE_PARAMETERIZED_DATA(values, all_values);

        auto [offsets, file_size, file_name, block_size, results]{values};

        auto blocks{
            get_blocks_between(move(offsets), file_size, file_name, block_size)
        };

        REQUIRE(blocks.size() == results.size());

        for (unsigned int i{0}; i < blocks.size(); i++) {
            CHECK(file_name == blocks[i]->file_name());
            CHECK(results[i].first == blocks[i]->offset());
            CHECK(results[i].second == blocks[i]->size());
        }
    }
}