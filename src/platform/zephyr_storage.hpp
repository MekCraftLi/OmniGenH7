/**
 *******************************************************************************
 * @file    zephyr_storage.hpp
 * @brief   Zephyr implementation of StoragePort using W25Q64
 *******************************************************************************
 * @attention
 *
 * This adapter exposes external NOR flash storage through the domain storage
 * abstraction.
 *
 *******************************************************************************
 * @note
 *
 * The W25Q64 device is configured through the board OCTOSPI devicetree node.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "ports/storage_port.hpp"

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace omnigen {

/**
 * @brief Zephyr 外部 NOR 存储适配类。
 *
 * 该类实现 `StoragePort`，将领域层存储请求同步转发给 W25Q64 支持驱动，并维护当前
 * 初始化状态供系统状态查询使用。对象不拥有 W25Q64 设备实例，设备生命周期由 Zephyr
 * 设备模型管理；对象也不创建专属线程，调用方需要自行保证并发访问策略。
 */
class ZephyrStorage : public StoragePort {
public:
    ZephyrStorage() = default;
    ~ZephyrStorage() override = default;

    Result<void> initialize() override;
    Result<void> read(const StorageReadRequest& request) override;
    Result<void> write(const StorageWriteRequest& request) override;
    Result<void> erase_sector(uint32_t address) override;
    Result<void> erase_range(const StorageEraseRangeRequest& request) override;
    bool ready() const override { return ready_; }

private:
    bool ready_ = false;
};

} // namespace omnigen
