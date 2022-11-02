# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
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
    "-Wno-format-nonliteral"
    "-Wno-conversion"
    "-DMG_ENABLE_DIRECTORY_LISTING=1"
    "-DMG_ENABLE_SSI=0"
)

if(MYMPD_ENABLE_IPV6)
  message("Mongoose: IPv6 is enabled")
  target_compile_options(mongoose
    PUBLIC
      "-DMG_ENABLE_IPV6=1"
  )
else()
  message("Mongoose: IPv6 is disabled")
endif()

if(MYMPD_ENABLE_SSL)
  message("Mongoose: SSL is enabled")
  target_compile_options(mongoose
    PUBLIC
      "-DMG_ENABLE_OPENSSL=1"
  )
  target_include_directories(mongoose SYSTEM
    PRIVATE
      ${OPENSSL_INCLUDE_DIR}
  )
else()
  message("Mongoose: SSL is disabled")
endif()
