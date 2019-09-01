cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)

project (mympd)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "6")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if("$ENV{ARCHLINUX}" MATCHES "true")
  set(WEBAPPS "/webapps")
else()
  set(WEBAPPS "")
endif()

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR "${WEBAPPS}/${PROJECT_NAME}")
  set(SUBDIRVAR "/${PROJECT_NAME}")
  set(SUBDIRVARLIB "/lib")
else() 
  set(SUBDIR "")
  set(SUBDIRVAR "")
  set(SUBDIRVARLIB "")
endif()

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(ASSETS_PATH "${CMAKE_INSTALL_PREFIX}/share${SUBDIR}/htdocs")
  set(DEBUG "OFF")
else()
  set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
  set(DEBUG "ON")
endif()

MESSAGE("ASSETS_PATH: ${ASSETS_PATH}")
set(ETC_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}${WEBAPPS}")
MESSAGE("ETC_PATH: ${ETC_PATH}")
set(PLUGIN_PATH "${CMAKE_INSTALL_FULL_LIBDIR}${SUBDIRVAR}")
MESSAGE("PLUGIN_PATH: ${PLUGIN_PATH}")
set(VARLIB_PATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRVARLIB}${SUBDIRVAR}")
MESSAGE("VARLIB_PATH: ${VARLIB_PATH}")
set(DATAROOT_PATH "${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}")
MESSAGE("DATAROOT_PATH: ${DATAROOT_PATH}")

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
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -DMG_ENABLE_SSL")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE -DMG_ENABLE_SSL")

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
	-fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
	-fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
	-fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
	-fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
endif()


#compiler flags for mympd src
file(GLOB MYMPD_SRC_FILES "src/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#compiler flags for mongoose.c
set_property(SOURCE dist/src/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-truncation")

set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

set(SOURCES
    src/main.c
    src/mpd_client.c
    src/web_server.c
    src/mympd_api.c
    src/list.c
    src/tiny_queue.c
    src/cert.c
    src/log.c
    src/config.c
    src/handle_options.c
    src/utility.c
    src/api.c
    dist/src/mongoose/mongoose.c
    dist/src/frozen/frozen.c
    dist/src/inih/ini.c
)

add_executable(mympd ${SOURCES})
target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

find_package(LibMediainfo)
if(LIBMEDIAINFO_FOUND)
  add_library(mympd_coverextract SHARED src/plugins/mympd_coverextract.cpp)

  install(TARGETS mympd_coverextract DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}${SUBDIRVAR}/)

  #testcoverextract executable
  add_executable(test_coverextract src/plugins/test_coverextract.c)
  target_link_libraries(test_coverextract ${CMAKE_DL_LIBS})
endif()

install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)
install(FILES htdocs/mympd.webmanifest DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES htdocs/robots.txt DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES dist/htdocs/index.html DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES dist/htdocs/sw.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES dist/htdocs/js/bootstrap-native-v4.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/js/mympd.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/js/keymap.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/js/i18n.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/css/bootstrap.min.css DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/css/)
install(FILES dist/htdocs/css/mympd.min.css DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/css/)
install(DIRECTORY htdocs/assets DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs)
