# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find pcre2
#
# PCRE2_FOUND
# PCRE2_INCLUDE_DIRS
# PCRE2_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_PCRE2 QUIET pcre2)

# Look for the header file
find_path(PCRE2_INCLUDE_DIR
    NAMES pcre2.h
    HINTS ${PC_PCRE2_INCLUDEDIR} ${PC_PCRE2_INCLUDE_DIRS}
)

# Look for the library
find_library(PCRE2_LIBRARY
    NAMES pcre2-8
    HINTS ${PC_PCRE2_LIBDIR} ${PC_PCRE2_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE2 DEFAULT_MSG
    PCRE2_LIBRARY PCRE2_INCLUDE_DIR
)

# Copy the results to the output variables
if(PCRE2_FOUND)
    set(PCRE2_LIBRARIES ${PCRE2_LIBRARY})
    set(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
else()
    set(PCRE2_LIBRARIES)
    set(PCRE2_INCLUDE_DIRS)
endif()

mark_as_advanced(PCRE2_INCLUDE_DIRS PCRE2_LIBRARIES)
