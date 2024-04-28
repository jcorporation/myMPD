#
# SPDX-License-Identifier: GPL-3.0-or-later
# myGPIOd (c) 2020-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/myGPIOd
#

add_library(libmygpio "")

configure_file(mygpio-common/compile_time.h.in "${PROJECT_BINARY_DIR}/dist/myGPIOd/compile_time.h")

target_include_directories(libmygpio PRIVATE
  "${PROJECT_SOURCE_DIR}/dist/myGPIOd"
  "${PROJECT_BINARY_DIR}/dist/myGPIOd"
)

target_sources(libmygpio
  PRIVATE
    libmygpio/src/buffer.c
    libmygpio/src/connection.c
    libmygpio/src/gpio_struct.c
    libmygpio/src/gpio.c
    libmygpio/src/gpioinfo.c
    libmygpio/src/gpiolist.c
    libmygpio/src/idle.c
    libmygpio/src/parser.c
    libmygpio/src/pair.c
    libmygpio/src/protocol.c
    libmygpio/src/socket.c
    libmygpio/src/util.c
    mygpio-common/util.c
)
