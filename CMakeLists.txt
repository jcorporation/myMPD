cmake_minimum_required(VERSION 2.6)

project (ympd C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    set(ASSETS_PATH "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/htdocs")
else()
    set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
endif()

option(WITH_MPD_HOST_CHANGE "Let users of the web frontend change the MPD Host" ON)
option(WITH_DYNAMIC_ASSETS "Serve assets dynamically (e.g for development/packaging)" ON)
option(WITH_IPV6 "enable IPv6 support" ON)
option(WITH_SSL "enable SSL support" ON)

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -pedantic")
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
    src/ympd.c
    src/mpd_client.c
    src/mongoose.c
    src/json_encode.c
)

if(NOT WITH_DYNAMIC_ASSETS)
    if(CMAKE_CROSSCOMPILING)
        set(MKDATA_EXE ${PROJECT_SOURCE_DIR}/tools/mkdata.pl)
    else()
        set(MKDATA_EXE $<TARGET_FILE:mkdata>)
        set(MKDATA_TARGET mkdata)
        add_executable(mkdata tools/mkdata.c)
    endif()

    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/assets.c
        COMMAND ${MKDATA_EXE} ${RESOURCES} > ${PROJECT_BINARY_DIR}/assets.c
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS ${RESOURCES} ${MKDATA_TARGET}
    )
    list(APPEND SOURCES src/http_server.c assets.c)
endif()

add_executable(ympd ${SOURCES})
target_link_libraries(ympd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES})

install(TARGETS ympd DESTINATION bin)
install(FILES ympd.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
if(WITH_DYNAMIC_ASSETS)
    install(DIRECTORY htdocs DESTINATION share/${PROJECT_NAME})
endif()
