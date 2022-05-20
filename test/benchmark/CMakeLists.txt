#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

project (benchmark C)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

set(BENCHMARK_SOURCES
  ../../dist/mongoose/mongoose.c
  ../../dist/rax/rax.c
  ../../dist/sds/sds.c
  ../../dist/tinymt/tinymt32.c
  ../../src/lib/list.c
  ../../src/lib/log.c
  ../../src/lib/random.c
  ../../src/lib/sds_extras.c
  main.c
)

#compiler flags for sds
#-Wno-stringop-overflow: gcc 11.2.0 bug
set_property(SOURCE ../../dist/sds/sds.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-stringop-overflow -Wno-conversion")

add_executable(benchmark ${BENCHMARK_SOURCES})
target_link_libraries(benchmark ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(benchmark ${MATH_LIB})
