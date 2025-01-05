# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find FLAC
#
# FLAC_FOUND
# FLAC_INCLUDE_DIRS
# FLAC_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_FLAC QUIET FLAC)

# Look for the header file
find_path(FLAC_INCLUDE_DIR
    NAMES FLAC/metadata.h
    HINTS ${PC_FLAC_INCLUDEDIR} ${PC_FLAC_INCLUDE_DIRS}
)

# Look for the library
find_library(FLAC_LIBRARY
    NAMES FLAC
    HINTS ${PC_FLAC_LIBDIR} ${PC_FLAC_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLAC DEFAULT_MSG
    FLAC_LIBRARY FLAC_INCLUDE_DIR
)

# Copy the results to the output variables
if(FLAC_FOUND)
    set(FLAC_LIBRARIES ${FLAC_LIBRARY})
    set(FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR})
else()
    set(FLAC_LIBRARIES)
    set(FLAC_INCLUDE_DIRS)
endif()

mark_as_advanced(FLAC_LIBRARY FLAC_INCLUDE_DIR)
