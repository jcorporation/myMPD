# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_subdirectory(libmympdclient)
add_subdirectory(mjson)
add_subdirectory(mpack)
add_subdirectory(mongoose)
add_subdirectory(rax)
add_subdirectory(sds)

if(MYMPD_ENABLE_MYGPIOD_STATIC)
    add_subdirectory(myGPIOd)
endif()
