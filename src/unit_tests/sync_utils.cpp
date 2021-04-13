#include "file_operator/sync_utils.h"
#include "message_utils.h"
#include "messages/basic.h"
#include "unit_tests/doctest_utils.h"
#include "messages/sync.pb.h"

#include <doctest.h>
#include <vector>

using namespace std;


TEST_SUITE("sync_utils") {
    TEST_CASE("get_block_pairs_between") {
        SUBCASE("the missing block is in the middle") {
            vector<BlockPair*> blocks{
                block_pair("A", 0, 0, 100),
                block_pair("A", 200, 233, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "A", 300, 333)};

            REQUIRE(in_between.size() == 1);

            CHECK(in_between[0]->file_name() == "A");
            CHECK(in_between[0]->offset_client() == 100);
            CHECK(in_between[0]->offset_server() == 100);
            CHECK(in_between[0]->size_client() == 100);
            CHECK(in_between[0]->size_server() == 133);
        }

        SUBCASE("the missing block is in front") {
            vector<BlockPair*> blocks{
                block_pair("B", 100, 133, 100),
                block_pair("B", 200, 233, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "B", 300, 333)};

            REQUIRE(in_between.size() == 1);

            CHECK(in_between[0]->file_name() == "B");
            CHECK(in_between[0]->offset_client() == 0);
            CHECK(in_between[0]->offset_server() == 0);
            CHECK(in_between[0]->size_client() == 100);
            CHECK(in_between[0]->size_server() == 133);
        }

        SUBCASE("the missing block is in the rear") {
            vector<BlockPair*> blocks{
                block_pair("C", 0, 0, 100),
                block_pair("C", 100, 100, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "C", 300, 333)};

            REQUIRE(in_between.size() == 1);

            CHECK(in_between[0]->file_name() == "C");
            CHECK(in_between[0]->offset_client() == 200);
            CHECK(in_between[0]->offset_server() == 200);
            CHECK(in_between[0]->size_client() == 100);
            CHECK(in_between[0]->size_server() == 133);
        }

        SUBCASE("there is no missing block") {
            vector<BlockPair*> blocks{
                block_pair("d", 0, 0, 100),
                block_pair("d", 100, 100, 100),
                block_pair("d", 200, 200, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "d", 300, 300)};

            CHECK(in_between.size() == 0);
        }

        SUBCASE("everything is missing") {
            vector<BlockPair*> blocks{};

            auto in_between{get_block_pairs_between(blocks, "e", 300, 333)};

            REQUIRE(in_between.size() == 1);

            CHECK(in_between[0]->file_name() == "e");
            CHECK(in_between[0]->offset_client() == 0);
            CHECK(in_between[0]->offset_server() == 0);
            CHECK(in_between[0]->size_client() == 300);
            CHECK(in_between[0]->size_server() == 333);
        }

        SUBCASE("the missing blocks are in the front, middle and rear") {
            vector<BlockPair*> blocks{
                block_pair("./a", 100, 100, 100),
                block_pair("./a", 300, 333, 100),
                block_pair("./a", 600, 556, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "./a", 800, 765)};

            REQUIRE(in_between.size() == 4);

            CHECK(in_between[0]->file_name() == "./a");
            CHECK(in_between[0]->offset_client() == 0);
            CHECK(in_between[0]->offset_server() == 0);
            CHECK(in_between[0]->size_client() == 100);
            CHECK(in_between[0]->size_server() == 100);

            CHECK(in_between[1]->file_name() == "./a");
            CHECK(in_between[1]->offset_client() == 200);
            CHECK(in_between[1]->offset_server() == 200);
            CHECK(in_between[1]->size_client() == 100);
            CHECK(in_between[1]->size_server() == 133);

            CHECK(in_between[2]->file_name() == "./a");
            CHECK(in_between[2]->offset_client() == 400);
            CHECK(in_between[2]->offset_server() == 433);
            CHECK(in_between[2]->size_client() == 200);
            CHECK(in_between[2]->size_server() == 123);

            CHECK(in_between[3]->file_name() == "./a");
            CHECK(in_between[3]->offset_client() == 700);
            CHECK(in_between[3]->offset_server() == 656);
            CHECK(in_between[3]->size_client() == 100);
            CHECK(in_between[3]->size_server() == 109);
        }

        SUBCASE("the file is of size 0") {
            vector<BlockPair*> blocks{};

            auto in_between{get_block_pairs_between(blocks, "e", 0, 0)};

            CHECK(in_between.size() == 0);
        }

        SUBCASE("the missing block is in front for one") {
            vector<BlockPair*> blocks{
                block_pair("B", 0, 133, 100),
                block_pair("B", 200, 233, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "B", 300, 333)};

            REQUIRE(in_between.size() == 2);

            CHECK(in_between[0]->file_name() == "B");
            CHECK(in_between[0]->offset_client() == 0);
            CHECK(in_between[0]->offset_server() == 0);
            CHECK(in_between[0]->size_client() == 0);
            CHECK(in_between[0]->size_server() == 133);

            CHECK(in_between[1]->file_name() == "B");
            CHECK(in_between[1]->offset_client() == 100);
            CHECK(in_between[1]->offset_server() == 233);
            CHECK(in_between[1]->size_client() == 100);
            CHECK(in_between[1]->size_server() == 0);
        }

        SUBCASE("the missing block is in the rear for one") {
            vector<BlockPair*> blocks{
                block_pair("C", 0, 0, 100),
                block_pair("C", 100, 233, 100)
            };

            auto in_between{get_block_pairs_between(blocks, "C", 300, 333)};

            REQUIRE(in_between.size() == 2);

            CHECK(in_between[0]->file_name() == "C");
            CHECK(in_between[0]->offset_client() == 100);
            CHECK(in_between[0]->offset_server() == 100);
            CHECK(in_between[0]->size_client() == 0);
            CHECK(in_between[0]->size_server() == 133);

            CHECK(in_between[1]->file_name() == "C");
            CHECK(in_between[1]->offset_client() == 200);
            CHECK(in_between[1]->offset_server() == 333);
            CHECK(in_between[1]->size_client() == 100);
            CHECK(in_between[1]->size_server() == 0);
        }

        SUBCASE("the file is of size 0 for one") {
            vector<BlockPair*> blocks{};

            auto in_between{get_block_pairs_between(blocks, "C", 0, 333)};

            REQUIRE(in_between.size() == 1);

            CHECK(in_between[0]->file_name() == "C");
            CHECK(in_between[0]->offset_client() == 0);
            CHECK(in_between[0]->offset_server() == 0);
            CHECK(in_between[0]->size_client() == 0);
            CHECK(in_between[0]->size_server() == 333);
        }
    }

    TEST_CASE("get_data_spaces") {
        SUBCASE("the missing block is in the middle") {
            vector<msg::Data> blocks{
                msg::Data("A", 0, 100, "abc"),
                msg::Data("A", 233, 110, "def")
            };

            auto all{get_data_spaces(move(blocks), "A", 343)};

            REQUIRE(all.size() == 3);

            CHECK(all[0].first.file_name == "A");
            CHECK(all[0].first.offset == 0);
            CHECK(all[0].first.size == 100);
            CHECK(all[0].first.data == "abc");
            CHECK(all[0].second);

            CHECK(all[1].first.file_name == "A");
            CHECK(all[1].first.offset == 100);
            CHECK(all[1].first.size == 133);
            CHECK(all[1].first.data == "");
            CHECK_FALSE(all[1].second);

            CHECK(all[2].first.file_name == "A");
            CHECK(all[2].first.offset == 233);
            CHECK(all[2].first.size == 110);
            CHECK(all[2].first.data == "def");
            CHECK(all[2].second);
        }

        SUBCASE("the missing block is in front") {
            vector<msg::Data> blocks{
                msg::Data("s", 100, 133, "def"),
                msg::Data("s", 233, 110, "xyza")
            };

            auto all{get_data_spaces(move(blocks), "s", 343)};

            REQUIRE(all.size() == 3);

            CHECK(all[0].first.file_name == "s");
            CHECK(all[0].first.offset == 0);
            CHECK(all[0].first.size == 100);
            CHECK(all[0].first.data == "");
            CHECK_FALSE(all[0].second);

            CHECK(all[1].first.file_name == "s");
            CHECK(all[1].first.offset == 100);
            CHECK(all[1].first.size == 133);
            CHECK(all[1].first.data == "def");
            CHECK(all[1].second);

            CHECK(all[2].first.file_name == "s");
            CHECK(all[2].first.offset == 233);
            CHECK(all[2].first.size == 110);
            CHECK(all[2].first.data == "xyza");
            CHECK(all[2].second);
        }

        SUBCASE("the missing block is in the rear") {
            vector<msg::Data> blocks{
                msg::Data("s", 0, 100, "a"),
                msg::Data("s", 100, 133, "def")
            };

            auto all{get_data_spaces(move(blocks), "s", 343)};

            REQUIRE(all.size() == 3);

            CHECK(all[0].first.file_name == "s");
            CHECK(all[0].first.offset == 0);
            CHECK(all[0].first.size == 100);
            CHECK(all[0].first.data == "a");
            CHECK(all[0].second);

            CHECK(all[1].first.file_name == "s");
            CHECK(all[1].first.offset == 100);
            CHECK(all[1].first.size == 133);
            CHECK(all[1].first.data == "def");
            CHECK(all[1].second);

            CHECK(all[2].first.file_name == "s");
            CHECK(all[2].first.offset == 233);
            CHECK(all[2].first.size == 110);
            CHECK(all[2].first.data == "");
            CHECK_FALSE(all[2].second);
        }

        SUBCASE("there is no missing block") {
            vector<msg::Data> blocks{
                msg::Data("s", 0, 100, "a"),
                msg::Data("s", 100, 133, "def")
            };

            auto all{get_data_spaces(move(blocks), "s", 233)};

            REQUIRE(all.size() == 2);

            CHECK(all[0].first.file_name == "s");
            CHECK(all[0].first.offset == 0);
            CHECK(all[0].first.size == 100);
            CHECK(all[0].first.data == "a");
            CHECK(all[0].second);

            CHECK(all[1].first.file_name == "s");
            CHECK(all[1].first.offset == 100);
            CHECK(all[1].first.size == 133);
            CHECK(all[1].first.data == "def");
            CHECK(all[1].second);
        }

        SUBCASE("everything is missing") {
            vector<msg::Data> blocks{};

            auto all{get_data_spaces(move(blocks), "./b/a", 233)};

            REQUIRE(all.size() == 1);

            CHECK(all[0].first.file_name == "./b/a");
            CHECK(all[0].first.offset == 0);
            CHECK(all[0].first.size == 233);
            CHECK(all[0].first.data == "");
            CHECK_FALSE(all[0].second);
        }

        SUBCASE("the missing blocks are in the front, middle and rear") {
            vector<msg::Data> blocks{
                msg::Data("A", 50, 100, "abc"),
                msg::Data("A", 233, 110, "def")
            };

            auto all{get_data_spaces(move(blocks), "A", 440)};

            REQUIRE(all.size() == 5);

            CHECK(all[0].first.file_name == "A");
            CHECK(all[0].first.offset == 0);
            CHECK(all[0].first.size == 50);
            CHECK(all[0].first.data == "");
            CHECK_FALSE(all[0].second);

            CHECK(all[1].first.file_name == "A");
            CHECK(all[1].first.offset == 50);
            CHECK(all[1].first.size == 100);
            CHECK(all[1].first.data == "abc");
            CHECK(all[1].second);

            CHECK(all[2].first.file_name == "A");
            CHECK(all[2].first.offset == 150);
            CHECK(all[2].first.size == 83);
            CHECK(all[2].first.data == "");
            CHECK_FALSE(all[2].second);

            CHECK(all[3].first.file_name == "A");
            CHECK(all[3].first.offset == 233);
            CHECK(all[3].first.size == 110);
            CHECK(all[3].first.data == "def");
            CHECK(all[3].second);

            CHECK(all[4].first.file_name == "A");
            CHECK(all[4].first.offset == 343);
            CHECK(all[4].first.size == 97);
            CHECK(all[4].first.data == "");
            CHECK_FALSE(all[4].second);
        }

        SUBCASE("the file is of size 0") {
            vector<msg::Data> blocks{};

            auto all{get_data_spaces(move(blocks), "./b/a", 0)};

            REQUIRE(all.size() == 0);
        }
    }
} 
