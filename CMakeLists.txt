cmake_minimum_required(VERSION 2.6)

project (mympd C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    set(ASSETS_PATH "/usr/share/${PROJECT_NAME}/htdocs")
    set(DEBUG "OFF")
else()
    set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
    set(DEBUG "ON")
endif()

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
set_property(SOURCE ${MYMPD_SRC_FILES} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

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
target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS mympd DESTINATION bin)
install(FILES contrib/mympd.1 DESTINATION share/man/man1)
install(PROGRAMS contrib/crcert.sh DESTINATION share/${PROJECT_NAME}/)
install(FILES contrib/mympd.service DESTINATION share/${PROJECT_NAME}/)
install(FILES contrib/mympd.conf.dist DESTINATION ../etc/${PROJECT_NAME}/)
install(FILES htdocs/mympd.webmanifest DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES htdocs/robots.txt DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES dist/htdocs/index.html DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES dist/htdocs/player.html DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES dist/htdocs/sw.min.js DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES dist/htdocs/js/player.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES dist/htdocs/js/bootstrap-native-v4.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES dist/htdocs/js/mympd.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES dist/htdocs/js/keymap.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES dist/htdocs/css/bootstrap.min.css DESTINATION share/${PROJECT_NAME}/htdocs/css/)
install(FILES dist/htdocs/css/mympd.min.css DESTINATION share/${PROJECT_NAME}/htdocs/css/)
install(DIRECTORY htdocs/assets DESTINATION share/${PROJECT_NAME}/htdocs)
install(DIRECTORY DESTINATION ../var/lib/${PROJECT_NAME}/pics)
install(DIRECTORY DESTINATION ../var/lib/${PROJECT_NAME}/tmp)
install(DIRECTORY DESTINATION ../var/lib/${PROJECT_NAME}/state)
install(DIRECTORY DESTINATION ../etc/${PROJECT_NAME}/syscmds)
install(DIRECTORY dist/smartpls DESTINATION ../var/lib/${PROJECT_NAME})
