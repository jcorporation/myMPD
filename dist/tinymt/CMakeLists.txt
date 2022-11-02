# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(tinymt "")

target_sources(tinymt
  PRIVATE
    tinymt32.c
  PUBLIC
    tinymt32.h
)
