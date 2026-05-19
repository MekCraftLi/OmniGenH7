/**
 *******************************************************************************
 * @file    shell_parse.hpp
 * @brief   Shared shell argument parsing helpers
 *******************************************************************************
 * @attention
 *
 * These helpers are intentionally small and header-only so diagnostics command
 * files can parse primitive values without sharing mutable state.
 *
 *******************************************************************************
 * @note
 *
 * All parsers require the whole input token to be consumed. Base-0 parsing is
 * used for addresses and byte-sized values so shell users may enter hex values.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/19
 * @version 1.0
 *******************************************************************************
 */
#pragma once

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

namespace omnigen {

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

static inline bool parse_shell_u32_arg(const char* text, uint32_t* out_value)
{
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno     = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT32_MAX)) {
        return false;
    }

    *out_value = static_cast<uint32_t>(value);
    return true;
}

static inline bool parse_shell_u8_arg(const char* text, uint8_t* out_value)
{
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno     = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT8_MAX)) {
        return false;
    }

    *out_value = static_cast<uint8_t>(value);
    return true;
}

static inline bool parse_shell_u16_arg(const char* text, uint16_t* out_value)
{
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno     = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT16_MAX)) {
        return false;
    }

    *out_value = static_cast<uint16_t>(value);
    return true;
}

static inline bool parse_shell_size_arg(const char* text, size_t* out_value)
{
    if (text == nullptr || out_value == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno     = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        return false;
    }

    *out_value = static_cast<size_t>(value);
    return true;
}

static inline bool parse_shell_addr_arg(const char* text, uint32_t* out_addr)
{
    if (text == nullptr || out_addr == nullptr || text[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    errno     = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > static_cast<unsigned long>(UINT32_MAX)) {
        return false;
    }

    *out_addr = static_cast<uint32_t>(value);
    return true;
}

} // namespace omnigen
