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

/**
 * @brief 显示矩形区域结构体。
 *
 * 使用左上角坐标和宽高描述屏幕上的一个矩形区域。坐标和尺寸均以像素为单位。
 */
struct DisplayRect {
    uint16_t x;      /**< 左上角 X 坐标。 */
    uint16_t y;      /**< 左上角 Y 坐标。 */
    uint16_t width;  /**< 矩形宽度，单位像素。 */
    uint16_t height; /**< 矩形高度，单位像素。 */
};

/**
 * @brief 显示位图拷贝请求结构体。
 *
 * 描述一次 RGB565 像素块写入操作。该结构不拥有像素数据，调用方必须保证
 * `pixels` 指向的缓冲区覆盖 `rect.width * rect.height` 个像素。
 */
struct DisplayBlitRequest {
    DisplayRect rect;       /**< 目标显示区域。 */
    const uint16_t* pixels; /**< RGB565 像素数据。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 显示端口抽象类。
 *
 * 该接口封装屏幕初始化、清屏、填充和像素块绘制操作。上层服务通过该接口绘制，
 * 不依赖具体 LCD 控制器或总线实现。
 */
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
