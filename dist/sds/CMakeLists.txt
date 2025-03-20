# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(sds "")

target_sources(sds
  PRIVATE
    sds.c
    sdsalloc.h
  PUBLIC
    sds.h
)

target_include_directories(sds
  PRIVATE
    ${PROJECT_SOURCE_DIR}
)

if(CMAKE_BUILD_TYPE MATCHES "(Debug|Release|RelWithDebInfo|MinSizeRel)")
  target_compile_options(sds
    PRIVATE
      "-Wno-format-nonliteral"
  )
endif()
