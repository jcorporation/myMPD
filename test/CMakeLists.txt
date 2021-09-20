#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 3.4)

project (test C)

find_package(Threads REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O1 -Wall -Werror -Wvla -Winit-self -Wuninitialized -Werror=array-bounds \
    -ggdb -fsanitize=address \
    -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
    -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
    -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
    -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr \
    -static-libasan")

configure_file(mympd_config_defs.h.in ${PROJECT_BINARY_DIR}/mympd_config_defs.h)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
include(CheckCSourceCompiles)

set(SOURCES
  ../src/lib/list.c
  ../src/lib/mem.c
  ../src/lib/mimetype.c
  ../src/lib/mympd_queue.c
  ../src/lib/log.c
  ../src/lib/random.c
  ../src/lib/sds_extras.c
  ../dist/src/sds/sds.c
  main.c
  tests/test_list.c
  tests/test_mimetype.c
  tests/test_mympd_queue.c
  tests/test_sds_extras.c
)

add_executable(test ${SOURCES})
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})
