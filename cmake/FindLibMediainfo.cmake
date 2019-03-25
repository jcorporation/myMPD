# - Try to find LibMediainfo
# Once done, this will define
#
#   LIBMEDIAINFO_FOUND - System has LibMediainfo
#   LIBMEDIAINFO_INCLUDE_DIR - The LibMediainfo include directories
#   LIBMEDIAINFO_LIBRARIES - The libraries needed to use LibMediainfo
#   LIBMEDIAINFO_DEFINITIONS - Compiler switches required for using LibMediainfo

find_package(PkgConfig)
pkg_check_modules(PC_LIBMEDIAINFO QUIET libmediainfo)
set(LIBMEDIAINFO_DEFINITIONS ${PC_LIBMEDIAINFO_CFLAGS_OTHER})

find_path(LIBMEDIAINFO_INCLUDE_DIR
    NAMES MediaInfoDLL/MediaInfoDLL.h
    HINTS ${PC_LIBMEDIAINFO_INCLUDEDIR} ${PC_LIBMEDIAINFO_INCLUDE_DIRS}
)

find_library(LIBMEDIAINFO_LIBRARY
    NAMES mediainfo
    HINTS ${PC_LIBMEDIAINFO_LIBDIR} ${PC_LIBMEDIAINFO_LIBRARY_DIRS}
)

set(LIBMEDIAINFO_LIBRARIES ${LIBMEDIAINFO_LIBRARY})
set(LIBMEDIAINFO_INCLUDE_DIRS ${LIBMEDIAINFO_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibMediainfo DEFAULT_MSG
    LIBMEDIAINFO_LIBRARY LIBMEDIAINFO_INCLUDE_DIR
)

mark_as_advanced(LIBMEDIAINFO_LIBRARY LIBMEDIAINFO_INCLUDE_DIR)

