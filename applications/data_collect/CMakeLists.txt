# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(data-collect)

target_include_directories(app PRIVATE include)
target_include_directories_ifdef(CONFIG_NET_UDP app PRIVATE src/cJSON)

target_sources(app PRIVATE
  src/main.c
  src/modbus/function.c
  src/modbus/dio.c
  src/modbus/init.c
  src/modbus/history.c
)


target_sources_ifdef(CONFIG_NET_TCP app PRIVATE src/modbus/tcp.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/modbus/udp.c src/cJSON/cJSON.c)
target_sources_ifdef(CONFIG_RTC app PRIVATE src/time.c)
target_sources_ifdef(CONFIG_SERIAL app PRIVATE src/modbus/rtu.c)
target_sources_ifdef(CONFIG_ADC app PRIVATE src/modbus/adc.c)
target_sources_ifdef(CONFIG_FILE_SYSTEM app PRIVATE src/fs.c)
