# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(mpack "")

target_sources(mpack
  PRIVATE
    mpack.c
  PUBLIC
    mpack.h
)
