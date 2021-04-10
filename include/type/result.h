#pragma once

#include "error.h"

#include <functional>
#include <utility>
#include <variant>


template<typename T>
class Result {
  private:
    std::variant<Error, T> value;
    bool has_ok;
  
    Result(T&& ok): value{std::move(ok)}, has_ok{true} {};
    Result(Error&& err): value{std::move(err)}, has_ok{false} {};

  public:

    static Result<T> ok(T&& ok) { return Result(std::move(ok)); }
    static Result<T> err(Error&& err) { return Result(std::move(err)); }

    bool is_ok() const { return has_ok; }
    bool is_err() const { return !has_ok; }

    operator bool() const { return is_ok(); }

    T get_ok() { return std::get<T>(std::move(value)); }
    Error get_err() { return std::get<Error>(std::move(value)); }

    template<typename U>
    Result<U> map(std::function<U(T)> functor) {
        if (is_ok()) {
            return Result<U>::ok(functor(get_ok()));
        }
        else {
            return Result<U>::err(get_err());
        }
    }

    template<typename U>
    Result<U> flat_map(std::function<Result<U>(T)> functor) {
        if (is_ok()) {
            return functor(get_ok());
        }
        else {
            return Result<U>::err(get_err());
        }
    }
};
