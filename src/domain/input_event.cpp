/**
 *******************************************************************************
 * @file    input_event.cpp
 * @brief   Input event factory implementations
 *******************************************************************************
 * @attention
 *
 * Factory helper functions for creating InputEvent instances.
 *
 *******************************************************************************
 * @note
 *
 * Events are typically created in ISR context, so factory functions
 * are kept simple and do not allocate memory.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/05/06
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/input_event.hpp"

namespace omnigen {

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

InputEvent InputEvent::make_button_press(ButtonId id, uint32_t timestamp)
{
    InputEvent evt{};
    evt.kind = InputEventKind::ButtonPress;
    evt.button_id = static_cast<uint8_t>(id);
    evt.timestamp_ms = timestamp;
    return evt;
}

InputEvent InputEvent::make_button_release(ButtonId id, uint32_t timestamp)
{
    InputEvent evt{};
    evt.kind = InputEventKind::ButtonRelease;
    evt.button_id = static_cast<uint8_t>(id);
    evt.timestamp_ms = timestamp;
    return evt;
}

InputEvent InputEvent::make_encoder_rotate(int16_t delta, uint32_t timestamp)
{
    InputEvent evt{};
    evt.kind = InputEventKind::EncoderRotate;
    evt.encoder_delta = delta;
    evt.timestamp_ms = timestamp;
    return evt;
}

InputEvent InputEvent::make_encoder_press(uint32_t timestamp)
{
    InputEvent evt{};
    evt.kind = InputEventKind::EncoderPress;
    evt.button_id = static_cast<uint8_t>(ButtonId::Encoder);
    evt.timestamp_ms = timestamp;
    return evt;
}

} // namespace omnigen