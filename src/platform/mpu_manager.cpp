/**
 *******************************************************************************
 * @file    mpu_manager.cpp
 * @brief   MPU policy manager implementation for board-specific memory attributes
 *******************************************************************************
 * @attention
 *
 * This file checks the generated Zephyr memory attribute table against board
 * assumptions for the FMC LCD memory window.
 *
 *******************************************************************************
 * @note
 *
 * The actual MPU programming is owned by Zephyr; this module only validates the
 * configured policy and reports mismatches during startup.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */
/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "platform/mpu_manager.hpp"

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>
#include <zephyr/logging/log.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/sys/util.h>

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

LOG_MODULE_REGISTER(mpu_manager, CONFIG_LOG_DEFAULT_LEVEL);

namespace omnigen {

#define FMC_LCD_NODE DT_NODELABEL(fmc_lcd_bank1)

static_assert(DT_NODE_EXISTS(FMC_LCD_NODE),
    "Missing fmc_lcd_bank1 node in board DTS");
static_assert(DT_PROP(FMC_LCD_NODE, zephyr_memory_attr) == DT_MEM_ARM(ATTR_MPU_IO),
    "fmc_lcd_bank1 must use ATTR_MPU_IO");
static_assert(DT_REG_ADDR(FMC_LCD_NODE) == 0x60000000,
    "fmc_lcd_bank1 base must be 0x60000000");
static_assert(DT_REG_SIZE(FMC_LCD_NODE) == 0x10000000UL,
    "fmc_lcd_bank1 size must be 256MB");

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

void MpuManager::init()
{
    const struct mem_attr_region_t* regions = nullptr;
    const size_t count = mem_attr_get_regions(&regions);

    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (regions[i].dt_addr != DT_REG_ADDR(FMC_LCD_NODE)) {
            continue;
        }

        found = true;
        if ((DT_MEM_ARM_GET(regions[i].dt_attr) == DT_MEM_ARM_MPU_IO)) {
            LOG_INF("MPU FMC region: 0x%08x size=0x%08x attr=Device(IO)",
                (uint32_t)regions[i].dt_addr, (uint32_t)regions[i].dt_size);
        } else {
            LOG_ERR("MPU FMC attr mismatch: 0x%x", regions[i].dt_attr);
        }
        break;
    }

    if (!found) {
        LOG_WRN("MPU FMC region not found in mem_attr table");
    }
}

} // namespace omnigen
