# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(mpack "")

target_sources(mpack
  PRIVATE
    mpack.c
    mpack-config.h
  PUBLIC
    mpack.h
)

target_include_directories(mpack
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)
