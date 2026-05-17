/**
 *******************************************************************************
 * @file    display_port.hpp
 * @brief   Display abstraction for board display output
 *******************************************************************************
 * @attention
 *
 * DisplayPort is a platform boundary for screen initialization and drawing
 * operations used by application services and diagnostics.
 *
 *******************************************************************************
 * @note
 *
 * Color values use RGB565 encoding unless a concrete implementation documents
 * another format.
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

#include <cstdint>

namespace omnigen {

/*-------- 2. data structures ----------------------------------------------------------------------------------------*/

struct DisplayRect {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
};

struct DisplayBlitRequest {
    DisplayRect rect;
    const uint16_t* pixels;
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class DisplayPort {
public:
    virtual ~DisplayPort() = default;

    virtual Result<void> mount() = 0;
    virtual Result<void> clear(uint16_t rgb565) = 0;
    virtual Result<void> fill(const DisplayRect& rect, uint16_t rgb565) = 0;
    virtual Result<void> blit(const DisplayBlitRequest& request) = 0;
    virtual bool mounted() const = 0;
};

} // namespace omnigen
