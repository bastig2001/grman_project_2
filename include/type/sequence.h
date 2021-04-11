#pragma once

#include "result.h"

#include <functional>
#include <optional>
#include <utility>
#include <variant>
#include <vector>


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

    template<class Iterator>
    Sequence(
        Iterator begin,
        Iterator end
    ): generator{
        [begin{std::move(begin)}, end{std::move(end)}](unsigned int index){
            if (begin + index < end) {
                return ResultVariant<T, bool>::ok(*(begin + index));
            }
            else {
                return ResultVariant<T, bool>::err(false); // no more values
            }
    }}
    {}

    template<typename U>
    Sequence<U> map(std::function<U(T)> functor) {
        return Sequence<U>(
            [generator{std::move(generator)}, functor{std::move(functor)}]
            (unsigned int index){
                return generator(index).template map<U>(functor);
            }
        );
    }

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

    Sequence<T> peek(std::function<void(const T&)> fn) {
        return map<T>([fn{std::move(fn)}](T value){
            fn(value);
            return value;
        });
    }

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

    template<typename U>
    U collect(U start_value, std::function<U(U, T)> fn) {
        U result{std::move(start_value)};

        for (unsigned int i{0};; i++) {
            auto value{generator(i)};

            if (value.is_ok()) {
                result = fn(std::move(result), value.get_ok());
            }
            else if (!value.get_err()) {
                // there are no more values
                return result;
            }
        }
    }

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
