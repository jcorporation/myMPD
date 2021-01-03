#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 2.6)

project (test C)

find_package(Threads REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O1 -Wall -Werror -Wuninitialized -ggdb")

set(SOURCES
  test.c
  ../dist/src/sds/sds.c 
  ../src/log.c 
  ../src/tiny_queue.c
  ../src/list.c
  ../src/random.c
  ../src/sds_extras.c
)

add_executable(test ${SOURCES})
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})

