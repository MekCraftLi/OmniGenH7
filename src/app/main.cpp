/**
 *******************************************************************************
 * @file    main.cpp
 * @brief   Application entry point
 *******************************************************************************
 * @attention
 *
 * Main entry point for OmniGen H7 signal generator.
 * Calls composition root for initialization, then enters idle loop.
 *
 *******************************************************************************
 * @note
 *
 * The main thread is kept minimal. Real work happens in service threads
 * created by the composition root.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/05
 * @version 1.0
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "composition_root.hpp"

#include <zephyr/kernel.h>

/* ------- function implement ----------------------------------------------------------------------------------------*/

int main(void)
{
    omnigen::init_system();

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
