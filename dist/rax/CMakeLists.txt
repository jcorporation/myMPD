# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(rax "")

target_sources(rax
  PRIVATE
    rax.c
    rax_malloc.h
  PUBLIC
    rax.h
)

target_compile_options(rax
  PRIVATE
    "-Wno-format-nonliteral"
    "-Wno-conversion"
    "-Wno-shadow"
    "-Wno-incompatible-pointer-types-discards-qualifiers"
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(rax
    PRIVATE
      "-Wno-discarded-qualifiers"
      "-Wno-use-after-free"
  )
endif()