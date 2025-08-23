# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find libmygpio
#
# LIBMYGPIO_FOUND
# LIBMYGPIO_INCLUDE_DIRS
# LIBMYGPIO_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_LIBMYGPIO QUIET libmygpio)

# Look for the header file
find_path(LIBMYGPIO_INCLUDE_DIR
    NAMES libmygpio.h
    HINTS ${PC_LIBMYGPIO_INCLUDEDIR} ${PC_LIBMYGPIO_INCLUDE_DIRS}
)

# Look for the library
find_library(LIBMYGPIO_LIBRARY
    NAMES mygpio
    HINTS ${PC_LIBMYGPIO_LIBDIR} ${PC_LIBMYGPIO_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBMYGPIO DEFAULT_MSG
    LIBMYGPIO_LIBRARY LIBMYGPIO_INCLUDE_DIR
)

# Copy the results to the output variables
if(LIBMYGPIO_FOUND)
    set(LIBMYGPIO_LIBRARIES ${LIBMYGPIO_LIBRARY})
    set(LIBMYGPIO_INCLUDE_DIRS ${LIBMYGPIO_INCLUDE_DIR})
else()
    set(LIBMYGPIO_LIBRARIES)
    set(LIBMYGPIO_INCLUDE_DIRS)
endif()

mark_as_advanced(LIBMYGPIO_INCLUDE_DIRS LIBMYGPIO_LIBRARIES)
