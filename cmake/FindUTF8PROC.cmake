# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find utf8proc
#
#  UTF8PROC_FOUND
#  UTF8PROC_INCLUDE_DIRS
#  UTF8PROC_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_UTF8PROC QUIET utf8proc)

# Look for the header file
find_path(UTF8PROC_INCLUDE_DIR
    NAMES utf8proc.h
    HINTS ${PC_UTF8PROC_INCLUDEDIR} ${PC_UTF8PROC_INCLUDE_DIRS}
)

# Look for the library
find_library(UTF8PROC_LIBRARY
    NAMES utf8proc
    HINTS ${PC_UTF8PROC_LIBDIR} ${PC_UTF8PROC_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UTF8PROC DEFAULT_MSG
    UTF8PROC_LIBRARY UTF8PROC_INCLUDE_DIR
)

# Copy the results to the output variables
if(UTF8PROC_FOUND)
    set(UTF8PROC_LIBRARIES ${UTF8PROC_LIBRARY})
    set(UTF8PROC_INCLUDE_DIRS ${UTF8PROC_INCLUDE_DIR})
else()
    set(UTF8PROC_LIBRARIES)
    set(UTF8PROC_INCLUDE_DIRS)
endif()

mark_as_advanced(UTF8PROC_INCLUDE_DIRS UTF8PROC_LIBRARIES)
