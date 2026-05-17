/*
 * SPDX-License-Identifier: Apache-2.0
 */

.dtcm_zephyr_stacks (NOLOAD) : SUBALIGN(8)
{
    . = ALIGN(8);
    *(".noinit.WEST_TOPDIR/zephyr/subsys/logging/log_core.c.*")
    *(".noinit.WEST_TOPDIR/zephyr/subsys/shell/backends/shell_uart.c.0")
    *(".noinit.WEST_TOPDIR/zephyr/kernel/init.c.*")
    *(".noinit.WEST_TOPDIR/zephyr/kernel/system_work_q.c.*")
    . = ALIGN(8);
} > DTCM