# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# Try to find Sphinx
#
# SPHINX_FOUND
# SPHINX_VERSION

include(FindPackageHandleStandardArgs)

find_program(SPHINX_EXECUTABLE NAMES sphinx-build)
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

find_package_handle_standard_args(
    SPHINX
    REQUIRED_VARS
        SPHINX_EXECUTABLE
    VERSION_VAR
        SPHINX_VERSION
    HANDLE_COMPONENTS
    HANDLE_VERSION_RANGE
)
