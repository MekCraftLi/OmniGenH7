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

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 输入事件类型枚举。
 *
 * 表示来自按键、编码器或主机通信的原始输入类别。该枚举处于输入层，不直接表达
 * 业务命令；后续路由层负责转换为 `SignalCommand`。
 */
enum class InputEventKind : uint8_t {
    None,            /**< 未指定事件。 */
    ButtonPress,     /**< 按键按下。 */
    ButtonRelease,   /**< 按键释放。 */
    ButtonLongPress, /**< 按键长按。 */
    EncoderRotate,   /**< 编码器旋转。 */
    EncoderPress,    /**< 编码器按下。 */
    HostCommand,     /**< 主机通信命令事件。 */
};

/**
 * @brief 按键标识枚举。
 *
 * 为板载物理按键和编码器按压建立逻辑编号，便于输入层屏蔽具体 GPIO 引脚。
 */
enum class ButtonId : uint8_t {
    None,    /**< 未指定按键。 */
    Start,   /**< 启动按键。 */
    Stop,    /**< 停止按键。 */
    Mode,    /**< 模式切换按键。 */
    Up,      /**< 上方向或增量按键。 */
    Down,    /**< 下方向或减量按键。 */
    Left,    /**< 左方向按键。 */
    Right,   /**< 右方向按键。 */
    Encoder, /**< 编码器按压。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 输入事件数据结构。
 *
 * 保存输入中断、轮询或通信回调产生的原始事件。该结构应尽量小且可复制，便于从
 * ISR 或驱动回调投递到事件队列。
 */
struct InputEvent {
    InputEventKind kind;  /**< 事件类型。 */
    uint8_t button_id;    /**< 按键逻辑编号，对应 `ButtonId`。 */
    int16_t encoder_delta; /**< 编码器增量，正负表示方向。 */
    uint32_t timestamp_ms; /**< 事件发生时间戳，单位 ms。 */

    static InputEvent make_button_press(ButtonId id, uint32_t timestamp);
    static InputEvent make_button_release(ButtonId id, uint32_t timestamp);
    static InputEvent make_encoder_rotate(int16_t delta, uint32_t timestamp);
    static InputEvent make_encoder_press(uint32_t timestamp);
};

} // namespace omnigen