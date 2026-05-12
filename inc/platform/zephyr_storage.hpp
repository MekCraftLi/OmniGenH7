/**
 *******************************************************************************
 * @file    zephyr_storage.hpp
 * @brief   Zephyr implementation of StoragePort using W25Q64
 *******************************************************************************
 */

#pragma once

#include "ports/storage_port.hpp"

namespace omnigen {

class ZephyrStorage : public StoragePort {
public:
    ZephyrStorage() = default;
    ~ZephyrStorage() override = default;

    Result<void> mount() override;
    Result<void> read(uint32_t address, void* data, size_t len) override;
    Result<void> write(uint32_t address, const void* data, size_t len) override;
    Result<void> erase_sector(uint32_t address) override;
    Result<void> erase_range(uint32_t address, size_t len) override;
};

} // namespace omnigen

