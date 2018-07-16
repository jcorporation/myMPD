cmake_minimum_required(VERSION 2.6)

project (mympd C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    set(ASSETS_PATH "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/htdocs")
    set(DEBUG "OFF")
else()
    set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
    set(DEBUG "ON")
endif()

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -pedantic -D MG_ENABLE_SSL -D MG_ENABLE_IPV6 -D MG_DISABLE_MQTT -D MG_DISABLE_MQTT_BROKER -D MG_DISABLE_DNS_SERVER -D MG_DISABLE_COAP -D MG_DISABLE_HTTP_CGI -D MG_DISABLE_HTTP_SSI -D MG_DISABLE_HTTP_WEBDAV")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -D_FORTIFY_SOURCE=2 -fstack-protector -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS NS_ENABLE_SSL)

set(SOURCES
    src/mympd.c
    src/mpd_client.c
    src/mongoose/mongoose.c
    src/frozen/frozen.c
)

add_executable(mympd ${SOURCES})
target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES})

install(TARGETS mympd DESTINATION bin)
install(FILES contrib/mympd.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
install(FILES htdocs/mympd.webmanifest DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES htdocs/index.html DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES htdocs/player.html DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES htdocs/sw.min.js DESTINATION share/${PROJECT_NAME}/htdocs/)
install(FILES htdocs/js/player.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES htdocs/js/bootstrap-native-v4.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES htdocs/js/mpd.min.js DESTINATION share/${PROJECT_NAME}/htdocs/js/)
install(FILES htdocs/css/bootstrap.min.css DESTINATION share/${PROJECT_NAME}/htdocs/css/)
install(FILES htdocs/css/mpd.min.css DESTINATION share/${PROJECT_NAME}/htdocs/css/)
install(DIRECTORY htdocs/assets DESTINATION share/${PROJECT_NAME}/htdocs)
install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME}/)
install(FILES contrib/options DESTINATION /etc/${PROJECT_NAME}/)
