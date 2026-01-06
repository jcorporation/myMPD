# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find libmpdclient
#
# LIBMPDCLIENT_FOUND
# LIBMPDCLIENT_INCLUDE_DIRS
# LIBMPDCLIENT_LIBRARIES
# LIBMPDCLIENT_VERSION

find_package(PkgConfig)
pkg_check_modules(PC_LIBMPDCLIENT QUIET libmpdclient)

# Look for the header file
find_path(LIBMPDCLIENT_INCLUDE_DIR
    NAMES mpd/client.h
    HINTS ${PC_LIBMPDCLIENT_INCLUDEDIR} ${PC_LIBMPDCLIENT_INCLUDE_DIRS}
)

# Look for the library
find_library(LIBMPDCLIENT_LIBRARY
    NAMES mpdclient
    HINTS ${PC_LIBMPDCLIENT_LIBDIR} ${PC_LIBMPDCLIENT_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBMPDCLIENT DEFAULT_MSG
    LIBMPDCLIENT_LIBRARY LIBMPDCLIENT_INCLUDE_DIR
)

if (LIBMPDCLIENT_FOUND)
  file(STRINGS "${LIBMPDCLIENT_INCLUDE_DIR}/mpd/version.h" version-file
    REGEX "#define[ \t]LIBMPDCLIENT_(MAJOR|MINOR|PATCH)_VERSION.*")
  if (NOT version-file)
    message(AUTHOR_WARNING "LIBMPDCLIENT_INCLUDE_DIR found, but mpd/version.h is missing")
  endif()
  list(GET version-file 0 major-line)
  list(GET version-file 1 minor-line)
  list(GET version-file 2 patch-line)
  string(REGEX REPLACE "^#define[ \t]+LIBMPDCLIENT_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" LIBMPDCLIENT_VERSION_MAJOR ${major-line})
  string(REGEX REPLACE "^#define[ \t]+LIBMPDCLIENT_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" LIBMPDCLIENT_VERSION_MINOR ${minor-line})
  string(REGEX REPLACE "^#define[ \t]+LIBMPDCLIENT_PATCH_VERSION[ \t]+([0-9]+)$" "\\1" LIBMPDCLIENT_VERSION_PATCH ${patch-line})
  set(LIBMPDCLIENT_VERSION ${LIBMPDCLIENT_VERSION_MAJOR}.${LIBMPDCLIENT_VERSION_MINOR}.${LIBMPDCLIENT_VERSION_PATCH})
endif()

# Copy the results to the output variables
if(LIBMPDCLIENT_FOUND)
    set(LIBMPDCLIENT_LIBRARIES ${LIBMPDCLIENT_LIBRARY})
    set(LIBMPDCLIENT_INCLUDE_DIRS ${LIBMPDCLIENT_INCLUDE_DIR})
else()
    set(LIBMPDCLIENT_LIBRARIES)
    set(LIBMPDCLIENT_INCLUDE_DIRS)
endif()

mark_as_advanced(LIBMPDCLIENT_INCLUDE_DIRS LIBMPDCLIENT_LIBRARIES LIBMPDCLIENT_VERSION)
