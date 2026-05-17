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

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief OmniGen 通用错误码枚举。
 *
 * 所有不使用异常的模块都通过该枚举表达失败原因。枚举值保持紧凑，适合在
 * 嵌入式日志、Shell 输出和总线响应中传递。
 */
enum class ErrorCode : uint8_t {
    Ok = 0,        /**< 操作成功。 */
    InvalidArgument, /**< 调用参数无效或超出允许范围。 */
    InvalidState, /**< 当前状态不允许执行该操作。 */
    Timeout,      /**< 等待硬件、总线或同步事件超时。 */
    IoError,      /**< 底层输入输出操作失败。 */
    Busy,         /**< 目标资源正忙，暂时无法处理请求。 */
    NoMemory,     /**< 内存、缓冲区或固定资源不足。 */
    Unsupported,  /**< 当前平台或模块不支持该功能。 */
    HardwareFault, /**< 硬件外设报告故障或初始化失败。 */
    DataCorrupted, /**< 数据校验失败或内容损坏。 */
    NotFound,     /**< 请求的对象、记录或设备不存在。 */
    BufferError,  /**< 缓冲区大小、对齐或生命周期不满足要求。 */
    ConfigError,  /**< 配置项、设备树或初始化参数错误。 */
    Unknown,      /**< 未分类错误，用于保底诊断。 */
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

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 携带返回值或错误码的轻量级结果类型。
 *
 * `Result<T>` 用于替代异常机制，调用方必须显式检查 `is_ok()` 或 `is_error()`。
 * 成功时保存一个 `T` 类型值，失败时保存 `ErrorCode`。该类型不依赖 STL，适合
 * 资源受限的嵌入式环境。
 *
 * @tparam T 成功路径返回的数据类型。
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
 * @brief 无返回值操作的结果类型特化。
 *
 * 该特化用于只需要表达成功或失败、不需要返回数据的函数，例如硬件初始化、
 * 命令提交和状态切换。内部仅保存错误码，避免为 `void` 构造无意义的存储。
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
