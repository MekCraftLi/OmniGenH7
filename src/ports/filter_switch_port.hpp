/**
 *******************************************************************************
 * @file    filter_switch_port.hpp
 * @brief   Filter bank switch abstraction for analog output conditioning
 *******************************************************************************
 * @attention
 *
 * FilterSwitchPort hides the board GPIO implementation for analog filter bank
 * selection and output mute control.
 *
 *******************************************************************************
 * @note
 *
 * Mode names are logical output conditioning modes. Concrete board wiring is
 * owned by platform implementations.
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

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/**
 * @brief 模拟输出滤波器档位枚举。
 *
 * 枚举值对应板级模拟滤波器组的逻辑选择状态。业务层只选择逻辑档位，不直接操作
 * SEL/EN/MUTE 引脚，也不持有任何 GPIO 资源。
 */
enum class FilterMode : uint8_t {
    Bypass,   /**< 旁路或宽带模式。 */
    Low25k,   /**< 25 kHz 低通滤波档。 */
    Mid125k,  /**< 125 kHz 低通滤波档。 */
    High225k, /**< 225 kHz 低通滤波档。 */
};

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 滤波器切换端口抽象类。
 *
 * 该接口封装模拟滤波器组的初始化、档位选择和输出静音控制。实现类负责具体
 * GPIO 时序，调用方只关心逻辑滤波模式。当前语义为同步设置硬件状态，端口对象不拥有
 * 独立执行线程；多个调用源同时操作时应由上层命令总线串行化。
 */
class FilterSwitchPort {
public:
    virtual ~FilterSwitchPort() = default;

    virtual Result<void> initialize() = 0;
    virtual Result<void> set_mode(FilterMode mode) = 0;
    virtual Result<void> set_mute(bool enabled) = 0;
    virtual bool ready() const = 0;
    virtual bool muted() const = 0;
    virtual FilterMode mode() const = 0;
};

} // namespace omnigen
