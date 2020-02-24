#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)

project (cli_tools C)
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
include(CheckCSourceCompiles)

if(${MEMCHECK} MATCHES "TRUE")
  #for use with valgrind
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -Werror -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
        -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
        -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
        -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
        -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
endif()

#compiler flags for non library files
file(GLOB SRC_FILES "*.c")
set_property(SOURCE ${SRC_FILES1} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
        -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#linker flags
set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

set(MYMPD_CONFIG_SOURCES
  mympd-config.c
  ../dist/src/sds/sds.c
  ../src/sds_extras.c
)

add_executable(mympd-config ${MYMPD_CONFIG_SOURCES})
target_link_libraries(mympd-config)
install(TARGETS mympd-config DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
