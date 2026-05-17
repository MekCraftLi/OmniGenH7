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

struct StorageReadRequest {
    uint32_t address;
    void* buffer;
    size_t length;
};

struct StorageWriteRequest {
    uint32_t address;
    const void* data;
    size_t length;
};

struct StorageEraseRangeRequest {
    uint32_t address;
    size_t length;
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace omnigen {

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

