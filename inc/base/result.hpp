/**
 *******************************************************************************
 * @file    result.hpp
 * @brief   Result type for error handling without exceptions
 *******************************************************************************
 * @attention
 *
 * Result<T> is a sum type that holds either a value of type T
 * or an ErrorCode. This enables explicit error handling without
 * exceptions, which is important for embedded systems.
 *
 *******************************************************************************
 * @note
 *
 * Design decisions:
 * - No exceptions: all error handling is explicit
 * - [[nodiscard]] forces callers to handle the result
 * - Minimal overhead: just the value and an error code
 * - Inspired by Rust's Result<T, E> and C++23's std::expected
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include <cstdint>
#include <utility>
#include <type_traits>

namespace omnigen {

/* ------- enum ------------------------------------------------------------------------------------------------------*/

/**
 * @brief Error codes for OmniGen operations.
 */
enum class ErrorCode : uint8_t {
    Ok = 0,
    InvalidArgument,
    InvalidState,
    Timeout,
    IoError,
    Busy,
    NoMemory,
    Unsupported,
    HardwareFault,
    DataCorrupted,
    NotFound,
    BufferError,
    ConfigError,
    Unknown,
};

/**
 * @brief Check if an error code indicates success.
 */
[[nodiscard]] inline constexpr bool is_ok(ErrorCode ec) noexcept {
    return ec == ErrorCode::Ok;
}

/**
 * @brief Check if an error code indicates failure.
 */
[[nodiscard]] inline constexpr bool is_error(ErrorCode ec) noexcept {
    return ec != ErrorCode::Ok;
}

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief A result type that holds either a value or an error.
 */
template <typename T>
class [[nodiscard]] Result {
public:
    using ValueType = T;

    constexpr Result(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : value_(std::move(value)), error_(ErrorCode::Ok), has_value_(true) {}

    constexpr Result(ErrorCode error) noexcept
        : value_(), error_(error), has_value_(false) {}

    [[nodiscard]] constexpr bool is_ok() const noexcept { return has_value_; }
    [[nodiscard]] constexpr bool is_error() const noexcept { return !has_value_; }

    [[nodiscard]] constexpr T& value() & noexcept { return value_; }
    [[nodiscard]] constexpr const T& value() const& noexcept { return value_; }
    [[nodiscard]] constexpr T&& value() && noexcept { return std::move(value_); }

    [[nodiscard]] constexpr ErrorCode error() const noexcept { return error_; }

    [[nodiscard]] constexpr T value_or(T default_value) const noexcept {
        return has_value_ ? value_ : std::move(default_value);
    }

private:
    T value_;
    ErrorCode error_;
    bool has_value_;
};

/**
 * @brief Specialization for void results.
 */
template <>
class [[nodiscard]] Result<void> {
public:
    using ValueType = void;

    constexpr Result() noexcept : error_(ErrorCode::Ok) {}
    constexpr Result(ErrorCode error) noexcept : error_(error) {}

    [[nodiscard]] constexpr bool is_ok() const noexcept { return error_ == ErrorCode::Ok; }
    [[nodiscard]] constexpr bool is_error() const noexcept { return error_ != ErrorCode::Ok; }
    [[nodiscard]] constexpr ErrorCode error() const noexcept { return error_; }

private:
    ErrorCode error_;
};

} // namespace omnigen
