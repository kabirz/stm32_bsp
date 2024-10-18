# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(probe-rs "--chip" "STM32F767IGTx")
board_runner_args(pyocd "--target" "STM32F767IGTx")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

# keep first
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
