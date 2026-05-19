/**
 *******************************************************************************
 * @file    display_port.hpp
 * @brief   Display abstraction for board display output
 *******************************************************************************
 * @attention
 *
 * DisplayPort hides LCD controller and bus details from the application layer.
 *
 *******************************************************************************
 * @note
 *
 * Pixel data uses RGB565 unless a concrete implementation documents another
 * format.
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
 * 使用左上角坐标和宽高描述屏幕上的一个矩形区域。该结构体只保存值，不拥有任何显示资源；
 * 坐标和尺寸均以像素为单位，由调用方保证区域落在目标显示范围内。
 */
struct DisplayRect {
    uint16_t x;      /**< 左上角 X 坐标。 */
    uint16_t y;      /**< 左上角 Y 坐标。 */
    uint16_t width;  /**< 矩形宽度，单位为像素。 */
    uint16_t height; /**< 矩形高度，单位为像素。 */
};

/**
 * @brief 显示位图拷贝请求结构体。
 *
 * 描述一次 RGB565 像素块写入操作。该结构体不拥有像素数据，调用方必须保证
 * `pixels` 指向的缓冲区覆盖 `rect.width * rect.height` 个像素，并在同步绘制调用期间保持有效。
 */
struct DisplayBlitRequest {
    DisplayRect rect;       /**< 目标显示区域。 */
    const uint16_t* pixels; /**< RGB565 像素数据，生命周期由调用方管理。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 显示端口抽象类。
 *
 * 该接口封装屏幕初始化、清屏、填充和像素块绘制操作。上层服务通过该接口绘制，
 * 不依赖具体 LCD 控制器或总线实现。接口不定义后台线程所有权；若实现使用 GUI 线程或
 * 显示刷新线程，必须在实现类中说明哪些函数可跨线程调用。
 */
class DisplayPort {
public:
    virtual ~DisplayPort() = default;

    virtual Result<void> initialize() = 0;
    virtual Result<void> clear(uint16_t rgb565) = 0;
    virtual Result<void> fill(const DisplayRect& rect, uint16_t rgb565) = 0;
    virtual Result<void> blit(const DisplayBlitRequest& request) = 0;
    virtual bool ready() const = 0;
};

} // namespace omnigen
