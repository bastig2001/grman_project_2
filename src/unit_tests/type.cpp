#include "type/error.h"
#include "type/result.h"
#include "type/sequence.h"
#include "unit_tests/doctest_utils.h"

#include <doctest.h>
#include <functional>
#include <optional>

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
                .map<int>([](int n){ return n * 3; })
            };

            REQUIRE(result.is_ok());
            CHECK(result.get_ok() == 12);
        }
        SUBCASE("error value") {
            auto result{
                Result<int>::err(Error{99, "XYZ"})
                .map<int>([](int n){ return n * 3; })
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
                .flat_map<int>([](int n){ 
                    return Result<int>::ok(n * 3); 
                })
            };

            REQUIRE(result.is_ok());
            CHECK(result.get_ok() == 12);
        }
        SUBCASE("error value and ok functor") {
            auto result{
                Result<int>::err(Error{99, "XYZ"})
                .flat_map<int>([](int n){ 
                    return Result<int>::ok(n * 3); 
                })
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 99);
            CHECK(err.msg == "XYZ");
        }
        SUBCASE("ok value and error functor") {
            auto result{
                Result<int>::ok(4)
                .flat_map<int>([](int){ 
                    return Result<int>::err(Error{42, "error"}); 
                })
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 42);
            CHECK(err.msg == "error");
        }
        SUBCASE("error value and error functor") {
            auto result{
                Result<int>::err(Error{99, "XYZ"})
                .flat_map<int>([](int){ 
                    return Result<int>::err(Error{42, "error"}); 
                })
            };

            REQUIRE(result.is_err());

            Error err{result.get_err()};
            CHECK(err.code == 99);
            CHECK(err.msg == "XYZ");
        }
    }
    TEST_CASE("result.apply") {
        SUBCASE("with one function") {
            SUBCASE("ok value") {
                auto result{Result<int>::ok(4)};

                bool applied{false};
                result.apply([&applied](int){ applied = true; });

                CHECK(applied);
            }
            SUBCASE("error value") {
                auto result{Result<int>::err(Error{99, "XYZ"})};

                bool applied{false};
                result.apply([&applied](int){ applied = true; });

                CHECK_FALSE(applied);
            }
        }
        SUBCASE("with two functions") {
            SUBCASE("ok value") {
                auto result{Result<int>::ok(4)};

                bool ok_applied{false};
                bool err_applied{false};
                result.apply(
                    [&ok_applied](int){ ok_applied = true; },
                    [&err_applied](Error){ err_applied = true; }
                );

                CHECK(ok_applied);
                CHECK_FALSE(err_applied);
            }
            SUBCASE("error value") {
                auto result{Result<int>::err(Error{99, "XYZ"})};

                bool ok_applied{false};
                bool err_applied{false};
                result.apply(
                    [&ok_applied](int){ ok_applied = true; },
                    [&err_applied](Error){ err_applied = true; }
                );

                CHECK_FALSE(ok_applied);
                CHECK(err_applied);
            }
        }
    }

    TEST_CASE("Sequence initialized with generator and collect") {
        Sequence<unsigned int> seq(
            [](unsigned int i){
                if (i <= 10) {
                    return ResultVariant<unsigned int, bool>::ok(move(i));
                }
                else {
                    return ResultVariant<unsigned int, bool>::err(false);
                }
            }
        );

        unsigned int sum{
            seq.collect<unsigned int>(
                0,
                [](unsigned int a, unsigned int b){
                    return a + b;
                }
        )};

        CHECK(sum == 55);
    }
    TEST_CASE("Sequence initialized with vector and for_each") {
        vector<int> values{1, 2, 3, 4, 5, 6};
        Sequence<int> seq(move(values));

        unsigned int sum{0};
        seq.for_each(
            [&sum](int i){
                sum += i;
            }
        );

        CHECK(sum == 21);
    }
    TEST_CASE("Sequence initialized with iterators and to_vector") {
        vector<int> values{1, 2, 3, 4, 5, 6};
        Sequence<int> seq(values.begin(), values.end());

        auto new_values{seq.to_vector()};

        REQUIRE(new_values.size() == values.size());

        for (unsigned int i{0}; i < values.size(); i++) {
            CHECK(new_values[i] == values[i]);
        }
    }
    TEST_CASE("sequence.map") {
        vector<int> values{1, 2, 3, 4, 5, 6};
        Sequence<int> seq(values.begin(), values.end());

        auto new_values{
            seq
            .map<int>([](int i){
                return i * 2;
            })
            .to_vector()
        };

        REQUIRE(new_values.size() == values.size());

        for (unsigned int i{0}; i < values.size(); i++) {
            CHECK(new_values[i] == values[i] * 2);
        }
    }
    TEST_CASE("sequence.where") {
        Sequence<unsigned int> seq(
            [](unsigned int i){
                if (i <= 10) {
                    return ResultVariant<unsigned int, bool>::ok(move(i));
                }
                else {
                    return ResultVariant<unsigned int, bool>::err(false);
                }
            }
        );

        unsigned int sum{
            seq
            .where(
                [](const unsigned int& i){
                    return i < 10;
                }
            )
            .collect<unsigned int>(
                0, 
                [](unsigned int a, unsigned int b){
                    return a + b;
                }
        )};

        CHECK(sum == 45);
    }
}
