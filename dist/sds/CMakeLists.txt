# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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

if(CMAKE_BUILD_TYPE MATCHES "(Release|Debug)")
  target_compile_options(sds
    PRIVATE
      "-Wno-unused-but-set-variable"
  )
endif()
