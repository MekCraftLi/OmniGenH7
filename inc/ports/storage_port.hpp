/**
 *******************************************************************************
 * @file    storage_port.hpp
 * @brief   Storage abstraction for external waveform/config storage
 *******************************************************************************
 */

#pragma once

#include "base/result.hpp"

#include <cstddef>
#include <cstdint>

namespace omnigen {

class StoragePort {
public:
    virtual ~StoragePort() = default;

    virtual Result<void> mount() = 0;
    virtual Result<void> read(uint32_t address, void* data, size_t len) = 0;
    virtual Result<void> write(uint32_t address, const void* data, size_t len) = 0;
    virtual Result<void> erase_sector(uint32_t address) = 0;
    virtual Result<void> erase_range(uint32_t address, size_t len) = 0;
};

} // namespace omnigen

