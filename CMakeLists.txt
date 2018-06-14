cmake_minimum_required(VERSION 2.6)

project (mympd C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    set(ASSETS_PATH "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/htdocs")
else()
    set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
    set(DEBUG ON)
endif()

option(WITH_IPV6 "enable IPv6 support" ON)
option(WITH_SSL "enable SSL support" ON)

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR})

include(CheckCSourceCompiles)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -pedantic -D_FORTIFY_SOURCE=2 -fstack-protector -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
if(WITH_IPV6)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS NS_ENABLE_IPV6)
endif()
if(WITH_SSL)
	find_package(OpenSSL REQUIRED)
	include_directories(${OPENSSL_INCLUDE_DIR})
#    list(APPEND LIB_LIST ${OPENSSL_LIBRARIES})
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS NS_ENABLE_SSL)
endif()

file(GLOB RESOURCES 
    RELATIVE ${PROJECT_SOURCE_DIR}
    htdocs/js/*
    htdocs/assets/*
    htdocs/css/*.css
    htdocs/fonts/*
    htdocs/index.html
    htdocs/player.html
)

set(SOURCES
    src/mympd.c
    src/mpd_client.c
    src/mongoose/mongoose.c
    src/json_encode.c
)

add_executable(mympd ${SOURCES})
target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES})

install(TARGETS mympd DESTINATION bin)
install(FILES mympd.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
install(DIRECTORY htdocs DESTINATION share/${PROJECT_NAME})
