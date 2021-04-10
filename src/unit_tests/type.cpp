#include "type/error.h"
#include "type/result.h"
#include "unit_tests/doctest_utils.h"

#include <doctest.h>
#include <functional>

using namespace std;


TEST_SUITE("type") {
    TEST_CASE("result.is_ok") {
        SUBCASE("ok value") {
            auto result{Result<int>::ok(3)};
            CHECK(result.is_ok());
        }
        SUBCASE("error value") {
            auto result{Result<int>::err(Error{1, ""})};
            CHECK_FALSE(result.is_ok());
        }
    }
    TEST_CASE("result.is_err") {
        SUBCASE("ok value") {
            auto result{Result<int>::ok(3)};
            CHECK_FALSE(result.is_err());
        }
        SUBCASE("error value") {
            auto result{Result<int>::err(Error{1, ""})};
            CHECK(result.is_err());
        }
    }
    TEST_CASE("result cast to bool") {
        SUBCASE("ok value") {
            auto result{Result<int>::ok(3)};
            CHECK(result);
        }
        SUBCASE("error value") {
            auto result{Result<int>::err(Error{1, ""})};
            CHECK_FALSE(result);
        }
    }
    TEST_CASE("result.get_ok") {
        auto result{Result<int>::ok(3)};
        CHECK(result.get_ok() == 3);
    }
    TEST_CASE("result.get_err") {
        auto result{Result<int>::err(Error{1, "abc"})};

        Error err{result.get_err()};
        CHECK(err.code == 1);
        CHECK(err.msg == "abc");
    }
    TEST_CASE("result.map") {
        SUBCASE("ok value") {
            auto result{
                Result<int>::ok(4)
                .map(function<int(int)>{[](int n){ return n * 3; }})
            };

            REQUIRE(result.is_ok());
            CHECK(result.get_ok() == 12);
        }
        SUBCASE("error value") {
            auto result{
                Result<int>::err(Error{99, "XYZ"})
                .map(function<int(int)>{[](int n){ return n * 3; }})
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 99);
            CHECK(err.msg == "XYZ");
        }
    }
    TEST_CASE("result.flat_map") {
        SUBCASE("ok value and ok functor") {
            auto result{
                Result<int>::ok(4)
                .flat_map(function<Result<int>(int)>{[](int n){ 
                    return Result<int>::ok(n * 3); 
                }})
            };

            REQUIRE(result.is_ok());
            CHECK(result.get_ok() == 12);
        }
        SUBCASE("error value and ok functor") {
            auto result{
                Result<int>::err(Error{99, "XYZ"})
                .flat_map(function<Result<int>(int)>{[](int n){ 
                    return Result<int>::ok(n * 3); 
                }})
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 99);
            CHECK(err.msg == "XYZ");
        }
        SUBCASE("ok value and error functor") {
            auto result{
                Result<int>::ok(4)
                .flat_map(function<Result<int>(int)>{[](int){ 
                    return Result<int>::err(Error{42, "error"}); 
                }})
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 42);
            CHECK(err.msg == "error");
        }
        SUBCASE("error value and error functor") {
            auto result{
                Result<int>::err(Error{99, "XYZ"})
                .flat_map(function<Result<int>(int)>{[](int){ 
                    return Result<int>::err(Error{42, "error"}); 
                }})
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 99);
            CHECK(err.msg == "XYZ");
        }
    }
}
