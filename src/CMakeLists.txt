cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
project(SleepyPicoProject)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()


add_executable(
  SleepyPico
  SleepyPico.cpp
  Sleep.cpp
  bme280_spi.cpp
  ss_oled.cpp
  ss_oled.c
  BitBang_I2C.c
)

pico_enable_stdio_uart(SleepyPico 1)
pico_enable_stdio_usb(SleepyPico 1)

pico_add_extra_outputs(SleepyPico)
target_link_libraries(SleepyPico hardware_sleep pico_stdlib pico_runtime hardware_spi hardware_i2c hardware_rtc hardware_rosc hardware_pll)
