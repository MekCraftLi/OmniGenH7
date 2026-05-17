/**
 *******************************************************************************
 * @file    storage_port.hpp
 * @brief   Storage abstraction for external waveform/config storage
 *******************************************************************************
 * @attention
 *
 * StoragePort is a domain-facing boundary; implementations must keep hardware
 * details outside services and domain code.
 *
 *******************************************************************************
 * @note
 *
 * Methods use byte addresses and byte lengths unless a concrete implementation
 * documents stricter alignment requirements.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "base/result.hpp"

#include <cstddef>
#include <cstdint>

/*-------- 2. data structures ----------------------------------------------------------------------------------------*/

/**
 * @brief 存储读取请求结构体。
 *
 * 描述一次从外部存储读取数据的操作。该结构不拥有缓冲区，调用方必须保证
 * `buffer` 可写且长度至少为 `length` 字节。
 */
struct StorageReadRequest {
    uint32_t address; /**< 读取起始字节地址。 */
    void* buffer;     /**< 接收数据的目标缓冲区。 */
    size_t length;    /**< 读取长度，单位字节。 */
};

/**
 * @brief 存储写入请求结构体。
 *
 * 描述一次向外部存储写入数据的操作。该结构不拥有数据缓冲区，调用方必须保证
 * `data` 在写入期间有效。
 */
struct StorageWriteRequest {
    uint32_t address; /**< 写入起始字节地址。 */
    const void* data; /**< 待写入数据缓冲区。 */
    size_t length;    /**< 写入长度，单位字节。 */
};

/**
 * @brief 存储范围擦除请求结构体。
 *
 * 描述一个按地址和长度指定的擦除范围。具体实现可根据芯片要求向扇区或块边界
 * 对齐。
 */
struct StorageEraseRangeRequest {
    uint32_t address; /**< 擦除起始字节地址。 */
    size_t length;    /**< 擦除长度，单位字节。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace omnigen {

/**
 * @brief 存储端口抽象类。
 *
 * 该接口为领域层提供外部存储能力，隐藏 W25Q64、OCTOSPI、Flash API 等平台细节。
 * 所有地址和长度默认以字节为单位。
 */
class StoragePort {
public:
    virtual ~StoragePort() = default;

    virtual Result<void> mount() = 0;
    virtual Result<void> read(const StorageReadRequest& request) = 0;
    virtual Result<void> write(const StorageWriteRequest& request) = 0;
    virtual Result<void> erase_sector(uint32_t address) = 0;
    virtual Result<void> erase_range(const StorageEraseRangeRequest& request) = 0;
    virtual bool mounted() const = 0;
};

} // namespace omnigen

