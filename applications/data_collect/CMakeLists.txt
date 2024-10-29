# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(data-collect)

target_sources(app PRIVATE
  src/main.c
  src/modbus/tcp.c
  src/modbus/function.c
  src/modbus/dio.c
  src/modbus/init.c
)

target_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MODBUS_SERIAL_ENABLED app PRIVATE
  src/modbus/rtu.c
)

target_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MODBUS_SERIAL_ENABLED app PRIVATE
  src/modbus/rtu.c
)
target_sources_ifdef(CONFIG_ADC app PRIVATE
  src/modbus/adc.c
)

target_include_directories(app PRIVATE
  include
)
