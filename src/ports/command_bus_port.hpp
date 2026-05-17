/**
 *******************************************************************************
 * @file    command_bus_port.hpp
 * @brief   Application command bus boundary
 *******************************************************************************
 * @attention
 *
 * CommandBusPort is the write-side application boundary for user, shell, host,
 * and input commands.
 *
 *******************************************************************************
 * @note
 *
 * The bus carries small typed commands only. Large payloads must use dedicated
 * ports such as StoragePort, DisplayPort, or WaveSinkPort.
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
#include "domain/signal_command.hpp"
#include "ports/filter_switch_port.hpp"

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

enum class AppCommandKind : uint8_t {
    None,
    Signal,
    Filter,
};

enum class FilterCommandKind : uint8_t {
    None,
    SetMode,
    SetMute,
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

struct FilterCommand {
    FilterCommandKind kind{FilterCommandKind::None};
    CommandSource source{CommandSource::None};
    uint32_t sequence{0U};
    FilterMode mode{FilterMode::Bypass};
    bool mute{false};

    static FilterCommand make_set_mode(CommandSource source, FilterMode mode);
    static FilterCommand make_set_mute(CommandSource source, bool enabled);
};

struct AppCommand {
    AppCommandKind kind{AppCommandKind::None};
    SignalCommand signal{};
    FilterCommand filter{};

    static AppCommand make_signal(const SignalCommand& command);
    static AppCommand make_filter(const FilterCommand& command);
};

class CommandBusPort {
public:
    virtual ~CommandBusPort() = default;

    virtual Result<void> submit(const AppCommand& command) = 0;
};

} // namespace omnigen
