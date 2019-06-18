cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)

project (mympd)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if("$ENV{ARCHLINUX}" MATCHES "true")
  set(WEBAPPS "/webapps")
else()
  set(WEBAPPS "")
endif()

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR "${WEBAPPS}/${PROJECT_NAME}")
  set(SUBDIRVAR "/${PROJECT_NAME}")
else() 
  set(SUBDIR "")
  set(SUBDIRVAR "")
endif()

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    set(ASSETS_PATH "${CMAKE_INSTALL_PREFIX}/share${SUBDIR}/htdocs")
    set(DEBUG "OFF")
else()
    set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
    set(DEBUG "ON")
endif()

MESSAGE("ASSETS_PATH: ${ASSETS_PATH}")
set(ETC_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}${SUBDIR}")
MESSAGE("ETC_PATH: ${ETC_PATH}")
set(PLUGIN_PATH "${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/lib")
MESSAGE("PLUGIN_PATH: ${PLUGIN_PATH}")

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    MESSAGE("++ 64 bit architecture") 
    set(PKGARCH64 "ON")
else() 
    MESSAGE("++ 32 bit architecture")
    set(PKGARCH64 "OFF")
endif()

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

configure_file(src/global.h.in ${PROJECT_BINARY_DIR}/global.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE -DMG_ENABLE_SSL")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
	-fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
	-fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
	-fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
	-fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")

#for use with valgrind, disable above 2 set commands and enable the following set commands
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -DMG_ENABLE_SSL")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")

#compiler flags for mympd src
file(GLOB MYMPD_SRC_FILES "src/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#compiler flags for mongoose.c
set_property(SOURCE dist/src/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-truncation")

set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

set(SOURCES
    src/main.c
    src/global.c
    src/mpd_client.c
    src/web_server.c
    src/mympd_api.c
    src/list.c
    src/tiny_queue.c
    dist/src/mongoose/mongoose.c
    dist/src/frozen/frozen.c
    dist/src/inih/ini.c
)

add_executable(mympd ${SOURCES})
target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})


find_package(LibMediainfo)
if(LIBMEDIAINFO_FOUND)
  add_library(mympd_coverextract SHARED src/plugins/mympd_coverextract.cpp)

  install(DIRECTORY DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/lib)
  install(TARGETS mympd_coverextract DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/lib/)

  #testcoverextract executable
  add_executable(test_coverextract src/plugins/test_coverextract.c)
  target_link_libraries(test_coverextract ${CMAKE_DL_LIBS})
endif()


install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES contrib/mympd.conf.dist DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}${SUBDIR}/)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}${SUBDIR}/syscmds)
install(PROGRAMS contrib/crcert.sh DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/)
install(FILES contrib/mympd.service DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/)
install(FILES htdocs/mympd.webmanifest DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES htdocs/robots.txt DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES dist/htdocs/index.html DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES dist/htdocs/sw.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/)
install(FILES dist/htdocs/js/bootstrap-native-v4.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/js/mympd.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/js/keymap.min.js DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/js/)
install(FILES dist/htdocs/css/bootstrap.min.css DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/css/)
install(FILES dist/htdocs/css/mympd.min.css DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs/css/)
install(DIRECTORY htdocs/assets DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}${SUBDIR}/htdocs)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib${SUBDIRVAR}/pics)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib${SUBDIRVAR}/state)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib${SUBDIRVAR}/covercache)
install(DIRECTORY dist/smartpls DESTINATION ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib${SUBDIRVAR})
