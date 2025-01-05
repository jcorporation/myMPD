# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find libid3tag
#
#  LIBID3TAG_FOUND
#  LIBID3TAG_INCLUDE_DIRS
#  LIBID3TAG_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_LIBID3TAG QUIET libid3tag)

# Look for the header file
find_path(LIBID3TAG_INCLUDE_DIR
    NAMES id3tag.h
    HINTS ${PC_LIBID3TAG_INCLUDEDIR} ${PC_LIBID3TAG_INCLUDE_DIRS}
)

# Look for the library
find_library(LIBID3TAG_LIBRARY
    NAMES id3tag
    HINTS ${PC_LIBID3TAG_LIBDIR} ${PC_LIBID3TAG_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBID3TAG DEFAULT_MSG
    LIBID3TAG_LIBRARY LIBID3TAG_INCLUDE_DIR
)

# Copy the results to the output variables
if(LIBID3TAG_FOUND)
    set(LIBID3TAG_LIBRARIES ${LIBID3TAG_LIBRARY})
    set(LIBID3TAG_INCLUDE_DIRS ${LIBID3TAG_INCLUDE_DIR})
else()
    set(LIBID3TAG_LIBRARIES)
    set(LIBID3TAG_INCLUDE_DIRS)
endif()

mark_as_advanced(LIBID3TAG_INCLUDE_DIRS LIBID3TAG_LIBRARIES)
