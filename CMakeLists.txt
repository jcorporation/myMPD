#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)

project (mympd)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "7")
set(CPACK_PACKAGE_VERSION_PATCH "2")

if(EXISTS "/etc/arch-release")
  set(WEBAPPS "/webapps/${PROJECT_NAME}")
else()
  set(WEBAPPS "")
endif()

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR_ETC "${WEBAPPS}")
  set(SUBDIR "/${PROJECT_NAME}")
  set(SUBDIRLIB "/lib")
else() 
  set(SUBDIR_ETC "")
  set(SUBDIR "")
  set(SUBDIRLIB "")
endif()

set(ETC_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}${SUBDIR_ETC}")
MESSAGE("ETC_PATH: ${ETC_PATH}")

MESSAGE("BIN_PATH: ${CMAKE_INSTALL_FULL_BINDIR}")

set(PLUGIN_PATH "${CMAKE_INSTALL_FULL_LIBDIR}${SUBDIR}")
MESSAGE("PLUGIN_PATH: ${PLUGIN_PATH}")

set(VARLIB_PATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(VARLIB_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
endif()
MESSAGE("VARLIB_PATH: ${VARLIB_PATH}")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(ASSETS_PATH "${VARLIB_PATH}/empty")
  set(DEBUG "OFF")
else()
  set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
  set(DEBUG "ON")
endif()
MESSAGE("ASSETS_PATH: ${ASSETS_PATH}")

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LibMediainfo)

if(LIBMEDIAINFO_FOUND)
  set(COVEREXTRACT "true")
else()
  set(COVEREXTRACT "false")
endif()

configure_file(src/config_defs.h.in ${PROJECT_BINARY_DIR}/config_defs.h)
configure_file(cmake/CopyConfig.cmake.in cmake/CopyConfig.cmake @ONLY)
configure_file(contrib/mympd.conf.in contrib/mympd.conf @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})

include(CheckCSourceCompiles)

if(${MEMCHECK} MATCHES TRUE)
  #for use with valgrind
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror -DMG_ENABLE_SSL")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu99 -Wall")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -Werror -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE -DMG_ENABLE_SSL")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
	-fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
	-fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
	-fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
	-fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
endif()


#compiler flags for mympd src
file(GLOB MYMPD_SRC_FILES1 "src/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES1} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

file(GLOB MYMPD_SRC_FILES2 "src/mpd_client/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES2} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

file(GLOB MYMPD_SRC_FILES3 "src/mympd_api/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES3} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#compiler flags for inih, issue#188
set_property(SOURCE dist/src/inih/ini.c PROPERTY COMPILE_FLAGS "-Wno-stringop-truncation")

#compiler flags for frozen
set_property(SOURCE dist/src/frozen/frozen.c PROPERTY COMPILE_FLAGS "-Wno-stringop-overflow")

#compiler flags for mongoose.c
set_property(SOURCE dist/src/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-truncation")

set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

set(SOURCES
    src/main.c
    src/api.c
    src/global.c
    src/list.c
    src/tiny_queue.c
    src/cert.c
    src/log.c
    src/config.c
    src/handle_options.c
    src/utility.c
    src/plugins.c
    src/sds_extras.c
    src/web_server.c
    src/mpd_client.c
    src/mpd_client/mpd_client_api.c
    src/mpd_client/mpd_client_cover.c
    src/mpd_client/mpd_client_browse.c
    src/mpd_client/mpd_client_features.c
    src/mpd_client/mpd_client_jukebox.c
    src/mpd_client/mpd_client_utility.c
    src/mpd_client/mpd_client_playlists.c
    src/mpd_client/mpd_client_queue.c
    src/mpd_client/mpd_client_search.c
    src/mpd_client/mpd_client_settings.c
    src/mpd_client/mpd_client_state.c
    src/mpd_client/mpd_client_stats.c
    src/mympd_api.c
    src/mympd_api/mympd_api_bookmarks.c
    src/mympd_api/mympd_api_utility.c
    src/mympd_api/mympd_api_settings.c
    src/mympd_api/mympd_api_syscmds.c
    dist/src/mongoose/mongoose.c
    dist/src/frozen/frozen.c
    dist/src/inih/ini.c
    dist/src/sds/sds.c
)

add_executable(mympd ${SOURCES})
target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

if(LIBMEDIAINFO_FOUND)
  add_library(mympd_coverextract SHARED src/plugins/mympd_coverextract.cpp)

  install(TARGETS mympd_coverextract DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}${SUBDIR}/)

  #testcoverextract executable
  add_executable(test_coverextract src/plugins/test_coverextract.c)
  target_link_libraries(test_coverextract ${CMAKE_DL_LIBS})
endif()

install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)
