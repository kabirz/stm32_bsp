# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(SNIPPET
  shell_en
  littlefs_flash
  imgmgr
)
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
target_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MODBUS_SERIAL_ENABLED app PRIVATE src/modbus/rtu.c)
target_sources_ifdef(CONFIG_ADC app PRIVATE src/modbus/adc.c)
target_sources_ifdef(CONFIG_FILE_SYSTEM app PRIVATE src/fs.c)
target_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT app PRIVATE src/fw_upgrade.c)

aux_source_directory(src/ftp FTP_SRC)
target_sources_ifdef(CONFIG_MB_FTP_SERVER app PRIVATE ${FTP_SRC})
target_compile_option_ifdef(CONFIG_MB_FTP_SERVER app PRIVATE -Wno-format-truncation)

if(CONFIG_HTTP_SERVER)
  zephyr_linker_sources(SECTIONS sections-rom.ld)
  zephyr_linker_section(NAME
  				http_resource_desc_test_http_service
  				KVMA RAM_REGION GROUP RODATA_REGION
  				SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  
  target_sources(app PRIVATE src/http_server.c)
  
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  
  foreach(web_resource index.html)
    generate_inc_file_for_target(
      app
      resources/${web_resource}
      ${gen_dir}/${web_resource}.gz.inc
      --gzip
    )
  endforeach()
endif()

