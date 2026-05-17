cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(omnigen_h7_app)

target_include_directories(app PRIVATE src drivers/include third_party/lvgl/demos)

set(LV_BUILD_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/lv_conf.h CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES n CACHE STRING "" FORCE)
set(CONFIG_LV_BUILD_DEMOS n CACHE STRING "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL n CACHE STRING "" FORCE)
set(CONFIG_LV_USE_PRIVATE_API n CACHE STRING "" FORCE)
add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)
target_compile_options(lvgl PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries(lvgl PUBLIC zephyr_interface)
target_link_libraries(app PRIVATE lvgl)

zephyr_linker_sources(SECTIONS src/platform/zephyr_dtcm_noinit.ld)

target_sources(app PRIVATE
    src/app/main.cpp
    src/app/composition_root.cpp
    src/domain/signal_validation.cpp
    src/domain/signal_command.cpp
    src/domain/input_event.cpp
    src/domain/system_status.cpp
    src/domain/waveform_synthesis.cpp
    src/ports/command_bus_port.cpp
    src/services/direct_command_bus.cpp
    src/services/direct_request_bus.cpp
    src/services/signal_engine.cpp
    src/platform/mock_wave_sink.cpp
    src/platform/mpu_manager.cpp
    src/platform/zephyr_display.cpp
    src/platform/zephyr_storage.cpp
    src/platform/zephyr_filter_switch.cpp
    src/platform/zephyr_wave_sink.cpp
    src/platform/lvgl_port.cpp
    third_party/lvgl/demos/stress/lv_demo_stress.c
    src/diagnostics/shell_commands.cpp
    drivers/output/dac_wave_sink.c
    drivers/output/ili9481_support.c
    drivers/storage/w25q64_support.c
)
