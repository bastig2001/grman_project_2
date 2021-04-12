#pragma once

#include "result.h"

#include <functional>
#include <optional>
#include <utility>
#include <variant>
#include <vector>


// A chainable and lazily executed sequence of values over a given source,
// only final methods cause the Sequence to be executed fully
template<typename T>
class Sequence {
  private:
    // T     ... the value you asked for
    // false ... there are no more values 
    // true  ... there is no value but continue (value might be filtered out)
    std::function<ResultVariant<T, bool>(unsigned int)> 
        generator;

  public:
    Sequence(
        std::function<ResultVariant<T, bool>(unsigned int)>
            generator
    ): generator{generator} 
    {}

    Sequence(
        std::vector<T>&& values
    ): generator{
        [values{std::move(values)}](unsigned int index){
            if (index < values.size()) {
                return ResultVariant<T, bool>::ok(std::move(values[index]));
            }
            else {
                return ResultVariant<T, bool>::err(false); // no more values
            }
    }}
    {}

    // applies the given functor to each value
    // and uses its result as new value
    template<typename U>
    Sequence<U> map(std::function<U(T)> functor) {
        return Sequence<U>(
            [generator{std::move(generator)}, functor{std::move(functor)}]
            (unsigned int index){
                return generator(index).template map<U>(functor);
            }
        );
    }

    // removes all values for which the predicate return false from the sequence
    Sequence<T> where(std::function<bool(const T&)> predicate) {
        return Sequence(
            [generator{std::move(generator)}, predicate{std::move(predicate)}]
            (unsigned int index){
                return 
                    generator(index)
                    .template flat_map<T>(
                        [predicate{std::move(predicate)}](T value){
                            if (predicate(value)) {
                                return 
                                    ResultVariant<T, bool>::ok(
                                        std::move(value)
                                    );
                            }
                            else {
                                return ResultVariant<T, bool>::err(true);
                            }
                        }
                    );
            }
        );
    }

    // executes the given function for each value without using it up
    Sequence<T> peek(std::function<void(const T&)> fn) {
        return map<T>([fn{std::move(fn)}](T value){
            fn(value);
            return value;
        });
    }

    // final method:
    // executes the given function for each value
    void for_each(std::function<void(T)> fn) {
        for (unsigned int i{0};; i++) {
            auto value{generator(i)};

            if (value.is_ok()) {
                fn(value.get_ok());
            }
            else if (!value.get_err()) {
                // there are no more values
                break;
            }
        }
    }

    // final method:
    // collects all values into one value which is returned 
    // with the given initialization value and collector function
    template<typename U>
    U collect(U start_value, std::function<U(U, T)> collector) {
        U result{std::move(start_value)};

        for (unsigned int i{0};; i++) {
            auto value{generator(i)};

            if (value.is_ok()) {
                result = collector(std::move(result), value.get_ok());
            }
            else if (!value.get_err()) {
                // there are no more values
                return result;
            }
        }
    }

    // final method:
    // collect all values into one vector which is returned
    std::vector<T> to_vector() {
        return 
            collect<std::vector<T>>(
                std::vector<T>{}, 
                [](std::vector<T> values, T value){
                    values.push_back(std::move(value));
                    return values;
                }
            );
    }
};
