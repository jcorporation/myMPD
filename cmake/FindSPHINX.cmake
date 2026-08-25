# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find Sphinx
#
# SPHINX_FOUND
# SPHINX_VERSION

include(FindPackageHandleStandardArgs)

if(SPHINX_DIR)
    find_program(SPHINX_EXECUTABLE
        NAMES         sphinx-build
        HINTS         "${SPHINX_DIR}"
        PATH_SUFFIXES bin
        NO_DEFAULT_PATH
    )
else()
    find_program(SPHINX_EXECUTABLE
        NAMES sphinx-build
    )
endif()
mark_as_advanced(SPHINX_EXECUTABLE)

if(SPHINX_EXECUTABLE)
    execute_process(
        COMMAND "${SPHINX_EXECUTABLE}" --version
        OUTPUT_VARIABLE _version
        ERROR_VARIABLE _version
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (_version MATCHES " ([0-9]+\\.[0-9]+\\.[0-9]+)$")
        set(SPHINX_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

# Set Python executable used by Sphinx
if(SPHINX_EXECUTABLE)
    set(SPHINX_PYTHON_EXECUTABLE "python3")
    file(STRINGS "${SPHINX_EXECUTABLE}" FIRST_LINE LIMIT_COUNT 1)
    if(FIRST_LINE MATCHES "^#! *(.*/python.*) *")
        set(SPHINX_PYTHON_EXECUTABLE "${CMAKE_MATCH_1}")
    endif()
endif()

find_package_handle_standard_args(
    SPHINX
    REQUIRED_VARS
        SPHINX_EXECUTABLE
    VERSION_VAR
        SPHINX_VERSION
    HANDLE_COMPONENTS
    HANDLE_VERSION_RANGE
)
