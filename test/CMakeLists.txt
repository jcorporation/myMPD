#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 2.6)

project (test_tiny_queue C)

find_package(Threads REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O1 -Wall -Werror -Wuninitialized -ggdb")

set(SOURCES
  test_tiny_queue.c
  ../dist/src/sds/sds.c 
  ../src/log.c 
  ../src/tiny_queue.c
)

add_executable(test_tiny_queue ${SOURCES})
target_link_libraries(test_tiny_queue ${CMAKE_THREAD_LIBS_INIT})
