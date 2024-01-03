# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(mjson "")

target_sources(mjson
  PRIVATE
    mjson.c
  PUBLIC
    mjson.h
)

target_compile_options(mjson
  PRIVATE
    "-DMJSON_ENABLE_PRINT=0"
    "-DMJSON_ENABLE_BASE64=0"
    "-DMJSON_ENABLE_RPC=0"
    "-DMJSON_ENABLE_PRETTY=0"
    "-DMJSON_ENABLE_MERGE=0"
)

if(CMAKE_BUILD_TYPE MATCHES "(Debug|Release|RelWithDebInfo|MinSizeRel)")
  if((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0.0"))
    #clang 15 compatibility
    target_compile_options(mjson
      PUBLIC
      "-Wno-unused-but-set-variable"
    )
    message("Enabling workaround for mjson and clang 15")
  endif()
endif()