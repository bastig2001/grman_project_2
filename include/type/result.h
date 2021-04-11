#pragma once

#include "error.h"

#include <functional>
#include <utility>
#include <variant>


template<typename Ok, typename Err>
class ResultVariant {
  private:
    std::variant<Ok, Err> value;
    bool has_ok;
  
    ResultVariant(Ok&& ok): value{std::move(ok)}, has_ok{true} {};
    ResultVariant(Err&& err): value{std::move(err)}, has_ok{false} {};

  public:
    ResultVariant(): value{}, has_ok{} {};

    static ResultVariant<Ok, Err> ok(Ok ok) { 
        return ResultVariant(std::move(ok)); 
    }
    static ResultVariant<Ok, Err> err(Err err) { 
        return ResultVariant(std::move(err)); 
    }

    bool is_ok() const { return has_ok; }
    bool is_err() const { return !has_ok; }

    operator bool() const { return is_ok(); }

    Ok get_ok() { return std::get<Ok>(std::move(value)); }
    Err get_err() { return std::get<Err>(std::move(value)); }

    Ok get_const_ok() const { return std::get<Ok>(value); }
    Err get_const_err() const { return std::get<Err>(value); }

    template<typename U>
    ResultVariant<U, Err> map(std::function<U(Ok)> functor) {
        if (is_ok()) {
            return ResultVariant<U, Err>::ok(functor(get_ok()));
        }
        else {
            return ResultVariant<U, Err>::err(get_err());
        }
    }

    template<typename U>
    ResultVariant<U, Err> flat_map(
        std::function<ResultVariant<U, Err>(Ok)> functor
    ) {
        if (is_ok()) {
            return functor(get_ok());
        }
        else {
            return ResultVariant<U, Err>::err(get_err());
        }
    }

    void apply(std::function<void(Ok)> fn) {
        if (is_ok()) {
            fn(get_ok());
        }
    }

    void apply(
        std::function<void(Ok)> ok_fn, 
        std::function<void(Err)> err_fn
    ) {
        if (is_ok()) {
            ok_fn(get_ok());
        }
        else {
            err_fn(get_err());
        }
    }

    ResultVariant<Ok, Err> peek(std::function<void(Ok)> fn) const {
        if (is_ok()) {
            fn(get_const_ok());

            return ResultVariant<Ok, Err>::ok(get_const_ok());
        }
        else {
            return ResultVariant<Ok, Err>::err(get_const_err());
        }
    }

    ResultVariant<Ok, Err> peek(
        std::function<void(Ok)> ok_fn, 
        std::function<void(Err)> err_fn
    ) const {
        if (is_ok()) {
            ok_fn(get_const_ok());

            return ResultVariant<Ok, Err>::ok(get_const_ok());
        }
        else {
            err_fn(get_const_err());

            return ResultVariant<Ok, Err>::err(get_const_err());
        }
    }

    Ok or_else(Ok value) {
        if (is_ok()) {
            return get_ok();
        }
        else {
            return value;
        }
    }
};

template<typename T>
using Result = ResultVariant<T, Error>;
