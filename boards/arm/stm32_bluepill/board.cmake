# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=stm32f103c8" "--frequency=4000000")
board_runner_args(jlink "--device=STM32F103C8" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
