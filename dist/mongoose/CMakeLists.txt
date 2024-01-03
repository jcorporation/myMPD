# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(mongoose "")

target_sources(mongoose
  PRIVATE
    mongoose.c
  PUBLIC
    mongoose.h
)

target_compile_options(mongoose
  PRIVATE
    "-DMG_ENABLE_DIRLIST=1"
    "-DMG_ENABLE_SSI=0"
    "-DMG_TLS=MG_TLS_OPENSSL"
)

if(CMAKE_BUILD_TYPE MATCHES "(Debug|Release|RelWithDebInfo|MinSizeRel)")
  target_compile_options(mongoose
    PRIVATE
      "-Wno-unused-parameter"
  )
endif()

if(MYMPD_ENABLE_IPV6)
  message("Mongoose: IPv6 is enabled")
  target_compile_options(mongoose
    PUBLIC
      "-DMG_ENABLE_IPV6=1"
      "-DMG_IPV6_V6ONLY=0"
  )
else()
  message("Mongoose: IPv6 is disabled")
endif()

target_include_directories(mongoose SYSTEM
  PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)
