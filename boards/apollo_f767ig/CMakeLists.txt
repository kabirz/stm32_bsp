if(CONFIG_DT_HAS_ST_STM32_ETHERNET_ENABLED)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
