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
 * - No STL dependencies for embedded compatibility
 * - C++11 compatible
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
inline constexpr bool is_ok(ErrorCode ec) noexcept {
    return ec == ErrorCode::Ok;
}

/**
 * @brief Check if an error code indicates failure.
 */
inline constexpr bool is_error(ErrorCode ec) noexcept {
    return ec != ErrorCode::Ok;
}

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief A result type that holds either a value or an error.
 *
 * Simplified implementation without STL dependencies.
 * Suitable for embedded environments with minimal C++ support.
 */
template <typename T>
class Result {
public:
    using ValueType = T;

    /**
     * @brief Construct a successful result with a value.
     */
    constexpr Result(T value) noexcept
        : value_(value), error_(ErrorCode::Ok), has_value_(true) {}

    /**
     * @brief Construct a failed result with an error code.
     */
    constexpr Result(ErrorCode error) noexcept
        : value_(), error_(error), has_value_(false) {}

    /**
     * @brief Check if this result contains a value.
     */
    constexpr bool is_ok() const noexcept {
        return has_value_;
    }

    /**
     * @brief Check if this result contains an error.
     */
    constexpr bool is_error() const noexcept {
        return !has_value_;
    }

    /**
     * @brief Get the value. UB if is_error().
     */
    T& value() noexcept {
        return value_;
    }

    /**
     * @brief Get the value (const). UB if is_error().
     */
    const T& value() const noexcept {
        return value_;
    }

    /**
     * @brief Get the error code.
     */
    constexpr ErrorCode error() const noexcept {
        return error_;
    }

    /**
     * @brief Get the value or a default.
     */
    T value_or(T default_value) const noexcept {
        return has_value_ ? value_ : default_value;
    }

private:
    T value_;
    ErrorCode error_;
    bool has_value_;
};

/**
 * @brief Specialization for void results (operations with no return value).
 */
template <>
class Result<void> {
public:
    using ValueType = void;

    /**
     * @brief Construct a successful result.
     */
    constexpr Result() noexcept : error_(ErrorCode::Ok) {}

    /**
     * @brief Construct a failed result with an error code.
     */
    constexpr Result(ErrorCode error) noexcept : error_(error) {}

    /**
     * @brief Check if this result is successful.
     */
    constexpr bool is_ok() const noexcept {
        return error_ == ErrorCode::Ok;
    }

    /**
     * @brief Check if this result contains an error.
     */
    constexpr bool is_error() const noexcept {
        return error_ != ErrorCode::Ok;
    }

    /**
     * @brief Get the error code.
     */
    constexpr ErrorCode error() const noexcept {
        return error_;
    }

private:
    ErrorCode error_;
};

} // namespace omnigen
