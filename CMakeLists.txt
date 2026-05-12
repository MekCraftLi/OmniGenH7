cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(omnigen_h7_app)

target_include_directories(app PRIVATE inc)

target_sources(app PRIVATE
    src/app/main.cpp
    src/app/composition_root.cpp
    src/domain/signal_validation.cpp
    src/domain/signal_command.cpp
    src/domain/input_event.cpp
    src/domain/waveform_synthesis.cpp
    src/services/signal_engine.cpp
    src/platform/mock_wave_sink.cpp
    src/platform/zephyr_wave_sink.cpp
    src/diagnostics/shell_commands.cpp
    drivers/output/dac_wave_sink.c
)