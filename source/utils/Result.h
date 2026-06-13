#pragma once

#include <string>
#include <variant>
#include <optional>

namespace rasterizer {

struct Error {
    std::string message;
    
    explicit Error(std::string msg) : message(std::move(msg)) {}
    
    Error(const Error&) = default;
    Error& operator=(const Error&) = default;
    Error(Error&&) noexcept = default;
    Error& operator=(Error&&) noexcept = default;
};

template<typename T>
class Result {
public:
    // Construct with success value
    Result(T value) : data_(std::move(value)) {}
    // Construct with error
    Result(Error error) : data_(std::move(error)) {}
    
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;
    
    // Check if result contains a value (success)
    bool isOk() const {
        return std::holds_alternative<T>(data_);
    }
    
    // Check if result contains an error (failure)
    bool isError() const {
        return std::holds_alternative<Error>(data_);
    }
    
    //Get the success value (undefined behavior if isError())
    const T& value() const {
        return std::get<T>(data_);
    }
    
    // Get the success value (undefined behavior if isError())
    T& value() {
        return std::get<T>(data_);
    }
    
    // Move the success value out (undefined behavior if isError())
    T takeValue() {
        return std::move(std::get<T>(data_));
    }
    
    // Get the error (undefined behavior if isOk())
    const Error& error() const {
        return std::get<Error>(data_);
    }
    
    // Get value or default if error
    T valueOr(T defaultValue) const {
        if (isOk()) {
            return value();
        }
        return defaultValue;
    }
    
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        if (isOk()) {
            return Result<U>(func(value()));
        }
        return Result<U>(error());
    }

private:
    std::variant<T, Error> data_;
};

template<>
class Result<void> {
public:
    Result() : error_(std::nullopt) {}
    
    Result(Error error) : error_(std::move(error)) {}
    
    bool isOk() const { return !error_.has_value(); }
    bool isError() const { return error_.has_value(); }
    
    const Error& error() const { return *error_; }

private:
    std::optional<Error> error_;
};

} 

