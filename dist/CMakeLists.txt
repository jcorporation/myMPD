# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_subdirectory(mpack)
add_subdirectory(mongoose)
add_subdirectory(rax)
add_subdirectory(sds)

if(MYMPD_EMBEDDED_LIBMPDCLIENT)
    add_subdirectory(libmpdclient)
endif()
