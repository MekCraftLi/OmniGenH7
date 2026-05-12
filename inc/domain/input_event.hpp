/**
 *******************************************************************************
 * @file    input_event.hpp
 * @brief   Input event definitions from buttons, encoder, and host
 *******************************************************************************
 * @attention
 *
 * InputEvent represents raw input from hardware (buttons, encoder) or
 * communication (UART host). InputRouter converts these to SignalCommand.
 *
 *******************************************************************************
 * @note
 *
 * Input flow:
 * GPIO EXTI / UART callback -> InputEvent -> queue -> InputRouter
 *
 * ISR only generates events, does not process business logic.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

#pragma once

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include <cstdint>

namespace omnigen {

/* ------- enum ------------------------------------------------------------------------------------------------------*/

/**
 * @brief Input event types.
 */
enum class InputEventKind : uint8_t {
    None,
    ButtonPress,
    ButtonRelease,
    ButtonLongPress,
    EncoderRotate,
    EncoderPress,
    HostCommand,
};

/**
 * @brief Button identifiers.
 */
enum class ButtonId : uint8_t {
    None,
    Start,
    Stop,
    Mode,
    Up,
    Down,
    Left,
    Right,
    Encoder,
};

/* ------- class prototypes ------------------------------------------------------------------------------------------*/

/**
 * @brief Input event from hardware or host.
 */
struct InputEvent {
    InputEventKind kind;
    uint8_t button_id;
    int16_t encoder_delta;
    uint32_t timestamp_ms;

    /* Factory helpers */
    static InputEvent make_button_press(ButtonId id, uint32_t timestamp);
    static InputEvent make_button_release(ButtonId id, uint32_t timestamp);
    static InputEvent make_encoder_rotate(int16_t delta, uint32_t timestamp);
    static InputEvent make_encoder_press(uint32_t timestamp);
};

} // namespace omnigen