/**
 *******************************************************************************
 * @file    mpu_manager.hpp
 * @brief   MPU policy manager for board-specific memory attributes
 *******************************************************************************
 * @attention
 *
 * The manager validates memory policy assumptions that are configured in board
 * devicetree and applied by Zephyr during boot.
 *
 *******************************************************************************
 * @note
 *
 * FMC LCD memory must remain mapped as Device-like IO memory on Cortex-M7.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

namespace omnigen {

/**
 * @brief MPU 策略检查管理类。
 *
 * 该类集中处理与 Cortex-M7 MPU 内存属性相关的启动期检查。实际 MPU 区域由
 * Zephyr 根据设备树配置完成，本类用于验证关键内存区域策略是否符合预期。
 */
class MpuManager {
public:
    /**
     * @brief Validate FMC MPU policy configured from devicetree.
     *
     * The actual MPU region programming is done by Zephyr at boot from
     * `zephyr,memory-attr` entries in the board DTS.
     */
    static void init();
};

} // namespace omnigen
