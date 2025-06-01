# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find SYSTEMD
#
#  SYSTEMD_FOUND
#  SYSTEMD_INCLUDE_DIRS
#  SYSTEMD_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_SYSTEMD QUIET SYSTEMD)

# Look for the header file
find_path(SYSTEMD_INCLUDE_DIR
    NAMES systemd/sd-daemon.h
    HINTS ${PC_SYSTEMD_INCLUDEDIR} ${PC_SYSTEMD_INCLUDE_DIRS}
)

# Look for the library
find_library(SYSTEMD_LIBRARY
    NAMES systemd
    HINTS ${PC_SYSTEMD_LIBDIR} ${PC_SYSTEMD_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SYSTEMD DEFAULT_MSG
    SYSTEMD_LIBRARY SYSTEMD_INCLUDE_DIR
)

# Copy the results to the output variables
if(SYSTEMD_FOUND)
    set(SYSTEMD_LIBRARIES ${SYSTEMD_LIBRARY})
    set(SYSTEMD_INCLUDE_DIRS ${SYSTEMD_INCLUDE_DIR})
else()
    set(SYSTEMD_LIBRARIES)
    set(SYSTEMD_INCLUDE_DIRS)
endif()

mark_as_advanced(SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES)
