/**
 *******************************************************************************
 * @file    nor_shell_commands.cpp
 * @brief   W25Q64 NOR flash shell diagnostics
 *******************************************************************************
 * @attention
 *
 * Provides the `nor` shell command group for manual W25Q64 probe, read, write,
 * erase, and verification flows.
 *
 *******************************************************************************
 * @note
 *
 * These commands operate on raw byte addresses through the W25Q64 support API.
 * They do not mount or manage a filesystem.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/19
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "diagnostics/shell_parse.hpp"
#include "drivers/w25q64_support.h"

#include <errno.h>
#include <string.h>
#include <zephyr/shell/shell.h>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

static constexpr size_t k_nor_shell_line_bytes = 16U;
static constexpr size_t k_nor_shell_rw_chunk   = 256U;
static constexpr size_t k_nor_shell_max_read   = 4096U;

/*-------- 3. internal helpers ---------------------------------------------------------------------------------------*/

static int nor_check_ready(const struct shell* sh)
{
    if (!w25q64_support_ready()) {
        shell_error(sh, "NOR not ready. Re-run: nor probe");
        return -ENODEV;
    }

    return 0;
}

/*-------- 4. command handlers ---------------------------------------------------------------------------------------*/

static int cmd_nor_probe(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret = w25q64_support_init();
    if (ret != 0) {
        shell_error(sh, "NOR init failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR probe success");
    return 0;
}

static int cmd_nor_info(const struct shell* sh, size_t argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t size_bytes = 0U;
    uint8_t jedec[3]    = {0U};
    size_t write_block  = w25q64_support_get_write_block_size();

    ret = w25q64_support_get_size(&size_bytes);
    if (ret != 0) {
        shell_error(sh, "NOR get size failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR info:");
    shell_print(sh, "  Size: %u bytes (%u KiB)", size_bytes, size_bytes / 1024U);
    shell_print(sh, "  Write block: %u bytes", static_cast<unsigned int>(write_block));
    shell_print(sh, "  Page size: 256 bytes");
    shell_print(sh, "  Sector size: 4096 bytes");

#if defined(CONFIG_FLASH_JESD216_API)
    ret = w25q64_support_read_jedec_id(jedec);
    if (ret == 0) {
        shell_print(sh, "  JEDEC ID: %02x %02x %02x", jedec[0], jedec[1], jedec[2]);
    } else {
        shell_warn(sh, "  JEDEC ID unavailable: %d", ret);
    }
#else
    shell_print(sh, "  JEDEC ID: disabled (CONFIG_FLASH_JESD216_API=n)");
#endif

    return 0;
}

static int cmd_nor_read(const struct shell* sh, size_t argc, char** argv)
{
    if (argc < 3U || argc > 4U) {
        shell_error(sh, "Usage: nor read <addr> <len> [dump]");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    size_t len    = 0U;
    if (!parse_shell_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_shell_size_arg(argv[2], &len) || len == 0U || len > k_nor_shell_max_read) {
        shell_error(sh, "Invalid len: %s (1..%u)", argv[2], static_cast<unsigned int>(k_nor_shell_max_read));
        return -EINVAL;
    }

    const bool do_dump = (argc == 4U) && (strcmp(argv[3], "dump") == 0);

    uint8_t buffer[k_nor_shell_rw_chunk] = {0U};
    size_t remain                        = len;
    uint32_t cursor                      = addr;

    shell_print(sh, "NOR read: addr=0x%08x len=%u", addr, static_cast<unsigned int>(len));

    while (remain > 0U) {
        size_t chunk = (remain > sizeof(buffer)) ? sizeof(buffer) : remain;
        ret = w25q64_support_read(cursor, buffer, chunk);
        if (ret != 0) {
            shell_error(sh, "Read failed at 0x%08x: %d", cursor, ret);
            return ret;
        }

        if (do_dump) {
            size_t offset = 0U;
            while (offset < chunk) {
                size_t line_len =
                    ((chunk - offset) > k_nor_shell_line_bytes) ? k_nor_shell_line_bytes : (chunk - offset);
                shell_fprintf(sh, SHELL_INFO, "0x%08x: ", cursor + static_cast<uint32_t>(offset));
                shell_hexdump(sh, &buffer[offset], line_len);
                offset += line_len;
            }
        }

        cursor += static_cast<uint32_t>(chunk);
        remain -= chunk;
    }

    shell_print(sh, "NOR read done");
    return 0;
}

static int cmd_nor_erase_sector(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 2U) {
        shell_error(sh, "Usage: nor erase_sector <addr>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    if (!parse_shell_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }

    ret = w25q64_support_erase_sector(addr);
    if (ret != 0) {
        shell_error(sh, "Erase sector failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR sector erased at 0x%08x", addr);
    return 0;
}

static int cmd_nor_erase_range(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 3U) {
        shell_error(sh, "Usage: nor erase_range <addr> <len>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    size_t len    = 0U;
    if (!parse_shell_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_shell_size_arg(argv[2], &len) || len == 0U) {
        shell_error(sh, "Invalid len: %s", argv[2]);
        return -EINVAL;
    }

    ret = w25q64_support_erase_range(addr, len);
    if (ret != 0) {
        shell_error(sh, "Erase range failed: %d", ret);
        return ret;
    }

    shell_print(sh, "NOR erase range done: addr=0x%08x len=%u", addr, static_cast<unsigned int>(len));
    return 0;
}

static int cmd_nor_fill(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 4U) {
        shell_error(sh, "Usage: nor fill <addr> <len> <byte>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    size_t len    = 0U;
    uint8_t value = 0U;
    if (!parse_shell_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }
    if (!parse_shell_size_arg(argv[2], &len) || len == 0U) {
        shell_error(sh, "Invalid len: %s", argv[2]);
        return -EINVAL;
    }
    if (!parse_shell_u8_arg(argv[3], &value)) {
        shell_error(sh, "Invalid byte: %s", argv[3]);
        return -EINVAL;
    }

    uint8_t pattern[k_nor_shell_rw_chunk];
    memset(pattern, value, sizeof(pattern));

    size_t remain   = len;
    uint32_t cursor = addr;
    while (remain > 0U) {
        size_t chunk = (remain > sizeof(pattern)) ? sizeof(pattern) : remain;
        ret = w25q64_support_write(cursor, pattern, chunk);
        if (ret != 0) {
            shell_error(sh, "Fill failed at 0x%08x: %d", cursor, ret);
            return ret;
        }
        cursor += static_cast<uint32_t>(chunk);
        remain -= chunk;
    }

    shell_print(sh, "NOR fill done: addr=0x%08x len=%u value=0x%02x", addr, static_cast<unsigned int>(len), value);
    return 0;
}

static int cmd_nor_write(const struct shell* sh, size_t argc, char** argv)
{
    if (argc < 3U) {
        shell_error(sh, "Usage: nor write <addr> <byte0> [byte1 ... byteN]");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    if (!parse_shell_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }

    const size_t data_len = argc - 2U;
    if (data_len > k_nor_shell_rw_chunk) {
        shell_error(sh, "Too many bytes: %u (max %u)", static_cast<unsigned int>(data_len),
                    static_cast<unsigned int>(k_nor_shell_rw_chunk));
        return -EINVAL;
    }

    uint8_t data[k_nor_shell_rw_chunk] = {0U};
    for (size_t i = 0U; i < data_len; ++i) {
        if (!parse_shell_u8_arg(argv[i + 2U], &data[i])) {
            shell_error(sh, "Invalid byte[%u]: %s", static_cast<unsigned int>(i), argv[i + 2U]);
            return -EINVAL;
        }
    }

    ret = w25q64_support_write(addr, data, data_len);
    if (ret != 0) {
        shell_error(sh, "Write failed: %d", ret);
        return ret;
    }

    uint8_t verify[k_nor_shell_rw_chunk] = {0U};
    ret = w25q64_support_read(addr, verify, data_len);
    if (ret != 0) {
        shell_warn(sh, "Readback failed: %d", ret);
        return ret;
    }

    if (memcmp(data, verify, data_len) != 0) {
        shell_error(sh, "Write verify mismatch");
        return -EIO;
    }

    shell_print(sh, "NOR write+verify done: addr=0x%08x len=%u", addr, static_cast<unsigned int>(data_len));
    return 0;
}

static int cmd_nor_verify(const struct shell* sh, size_t argc, char** argv)
{
    if (argc < 3U) {
        shell_error(sh, "Usage: nor verify <addr> <byte0> [byte1 ... byteN]");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t addr = 0U;
    if (!parse_shell_addr_arg(argv[1], &addr)) {
        shell_error(sh, "Invalid addr: %s", argv[1]);
        return -EINVAL;
    }

    const size_t data_len = argc - 2U;
    if (data_len > k_nor_shell_rw_chunk) {
        shell_error(sh, "Too many bytes: %u (max %u)", static_cast<unsigned int>(data_len),
                    static_cast<unsigned int>(k_nor_shell_rw_chunk));
        return -EINVAL;
    }

    uint8_t expected[k_nor_shell_rw_chunk] = {0U};
    for (size_t i = 0U; i < data_len; ++i) {
        if (!parse_shell_u8_arg(argv[i + 2U], &expected[i])) {
            shell_error(sh, "Invalid byte[%u]: %s", static_cast<unsigned int>(i), argv[i + 2U]);
            return -EINVAL;
        }
    }

    uint8_t actual[k_nor_shell_rw_chunk] = {0U};
    ret = w25q64_support_read(addr, actual, data_len);
    if (ret != 0) {
        shell_error(sh, "Read failed: %d", ret);
        return ret;
    }

    if (memcmp(expected, actual, data_len) != 0) {
        shell_error(sh, "Verify failed at addr=0x%08x", addr);
        shell_print(sh, "Expected:");
        shell_hexdump(sh, expected, data_len);
        shell_print(sh, "Actual:");
        shell_hexdump(sh, actual, data_len);
        return -EIO;
    }

    shell_print(sh, "NOR verify pass: addr=0x%08x len=%u", addr, static_cast<unsigned int>(data_len));
    return 0;
}

static int cmd_nor_test(const struct shell* sh, size_t argc, char** argv)
{
    if (argc != 2U) {
        shell_error(sh, "Usage: nor test <sector_addr>");
        return -EINVAL;
    }

    int ret = nor_check_ready(sh);
    if (ret != 0) {
        return ret;
    }

    uint32_t sector_addr = 0U;
    if (!parse_shell_addr_arg(argv[1], &sector_addr)) {
        shell_error(sh, "Invalid sector_addr: %s", argv[1]);
        return -EINVAL;
    }

    uint8_t tx[k_nor_shell_rw_chunk] = {0U};
    uint8_t rx[k_nor_shell_rw_chunk] = {0U};
    for (size_t i = 0U; i < sizeof(tx); ++i) {
        tx[i] = static_cast<uint8_t>((i ^ 0x5aU) & 0xffU);
    }

    ret = w25q64_support_erase_sector(sector_addr);
    if (ret != 0) {
        shell_error(sh, "Test erase failed: %d", ret);
        return ret;
    }

    ret = w25q64_support_write(sector_addr, tx, sizeof(tx));
    if (ret != 0) {
        shell_error(sh, "Test write failed: %d", ret);
        return ret;
    }

    ret = w25q64_support_read(sector_addr, rx, sizeof(rx));
    if (ret != 0) {
        shell_error(sh, "Test read failed: %d", ret);
        return ret;
    }

    if (memcmp(tx, rx, sizeof(tx)) != 0) {
        shell_error(sh, "Test mismatch");
        return -EIO;
    }

    shell_print(sh, "NOR R/W test pass at sector 0x%08x (%u bytes)", sector_addr,
                static_cast<unsigned int>(sizeof(tx)));
    return 0;
}

/*-------- 5. shell command definitions ------------------------------------------------------------------------------*/

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_nor,
    SHELL_CMD(probe, NULL, "Probe/initialize NOR device", cmd_nor_probe),
    SHELL_CMD(info, NULL, "Show NOR capacity and JEDEC info", cmd_nor_info),
    SHELL_CMD(read, NULL, "Read NOR: nor read <addr> <len> [dump]", cmd_nor_read),
    SHELL_CMD(write, NULL, "Write bytes: nor write <addr> <b0> [b1 ...]", cmd_nor_write),
    SHELL_CMD(verify, NULL, "Verify bytes: nor verify <addr> <b0> [b1 ...]", cmd_nor_verify),
    SHELL_CMD(fill, NULL, "Fill bytes: nor fill <addr> <len> <byte>", cmd_nor_fill),
    SHELL_CMD(erase_sector, NULL, "Erase 4KB sector: nor erase_sector <addr>", cmd_nor_erase_sector),
    SHELL_CMD(erase_range, NULL, "Erase range: nor erase_range <addr> <len>", cmd_nor_erase_range),
    SHELL_CMD(test, NULL, "R/W test: nor test <sector_addr>", cmd_nor_test),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nor, &sub_nor, "NOR flash commands", NULL);

} // namespace omnigen
