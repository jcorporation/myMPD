# - Try to find FLAC
# Once done, this will define
#
#   FLAC_FOUND - System has flac
#   FLAC_INCLUDE_DIR - The flac include directories
#   FLAC_LIBRARIES - The libraries needed to use flac
#   FLAC_DEFINITIONS - Compiler witches required for using flac

find_package(PkgConfig)
pkg_check_modules(PC_FLAC QUIET libflac)
set(FLAC_DEFINITIONS ${PC_FLAC_CFLAGS_OTHER})

find_path(FLAC_INCLUDE_DIR
    NAMES FLAC/metadata.h
    HINTS ${PC_FLAC_INCLUDEDIR} ${PC_FLAC_INCLUDE_DIRS}
)

find_library(FLAC_LIBRARY
    NAMES FLAC
    HINTS ${PC_FLAC_LIBDIR} ${PC_FLAC_LIBRARY_DIRS}
)

set(FLAC_LIBRARIES ${FLAC_LIBRARY})
set(FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLAC DEFAULT_MSG
    FLAC_LIBRARY FLAC_INCLUDE_DIR
)

mark_as_advanced(FLAC_LIBRARY FLAC_INCLUDE_DIR)
