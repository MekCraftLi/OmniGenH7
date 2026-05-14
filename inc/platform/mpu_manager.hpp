/**
 *******************************************************************************
 * @file    mpu_manager.hpp
 * @brief   MPU policy manager for board-specific memory attributes
 *******************************************************************************
 */

#pragma once

namespace omnigen {

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
