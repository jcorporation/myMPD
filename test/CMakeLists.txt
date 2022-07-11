#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 3.4)

project (test C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../cmake/")

find_package(Threads REQUIRED)
find_library(MATH_LIB m REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PCRE2 REQUIRED)

set(TEST_SOURCES
  ../dist/mjson/mjson.c
  ../dist/mongoose/mongoose.c
  ../dist/rax/rax.c
  ../dist/sds/sds.c
  ../dist/tinymt/tinymt32.c
  ../src/lib/api.c
  ../src/lib/filehandler.c
  ../src/lib/http_client.c
  ../src/lib/jsonrpc.c
  ../src/lib/list.c
  ../src/lib/log.c
  ../src/lib/lua_mympd_state.c
  ../src/lib/m3u.c
  ../src/lib/mimetype.c
  ../src/lib/mympd_queue.c
  ../src/lib/mympd_state.c
  ../src/lib/random.c
  ../src/lib/sds_extras.c
  ../src/lib/state_files.c
  ../src/lib/utility.c
  ../src/lib/validate.c
  ../src/mpd_client/mpd_client_errorhandler.c
  ../src/mpd_client/mpd_client_search.c
  ../src/mpd_client/mpd_client_search_local.c
  ../src/mpd_client/mpd_client_sticker.c
  ../src/mpd_client/mpd_client_tags.c
  ../src/mpd_client/mpd_client_jukebox.c
  ../src/mpd_client/mpd_client_volume.c
  ../src/mympd_api/mympd_api_extra_media.c
  ../src/mympd_api/mympd_api_status.c
  ../src/mympd_api/mympd_api_timer.c
  ../src/mympd_api/mympd_api_timer_handlers.c
  ../src/mympd_api/mympd_api_queue.c
  ../src/mympd_api/mympd_api_webradios.c
  main.c
  tests/test_api.c
  tests/test_http_client.c
  tests/test_jsonrpc.c
  tests/test_list.c
  tests/test_m3u.c
  tests/test_mpd_client_tags.c
  tests/test_mimetype.c
  tests/test_mympd_queue.c
  tests/test_random.c
  tests/test_sds_extras.c
  tests/test_state_files.c
  tests/test_utility.c
  tests/test_validate.c
)

set(LIBMPDCLIENT_SOURCES
  ../dist/libmpdclient/src/albumart.c
  ../dist/libmpdclient/src/async.c
  ../dist/libmpdclient/src/audio_format.c
  ../dist/libmpdclient/src/binary.c
  ../dist/libmpdclient/src/capabilities.c
  ../dist/libmpdclient/src/cmessage.c
  ../dist/libmpdclient/src/cmount.c
  ../dist/libmpdclient/src/cneighbor.c
  ../dist/libmpdclient/src/connection.c
  ../dist/libmpdclient/src/coutput.c
  ../dist/libmpdclient/src/cpartition.c
  ../dist/libmpdclient/src/cplaylist.c
  ../dist/libmpdclient/src/cstats.c
  ../dist/libmpdclient/src/cstatus.c
  ../dist/libmpdclient/src/database.c
  ../dist/libmpdclient/src/directory.c
  ../dist/libmpdclient/src/entity.c
  ../dist/libmpdclient/src/error.c
  ../dist/libmpdclient/src/fd_util.c
  ../dist/libmpdclient/src/fingerprint.c
  ../dist/libmpdclient/src/idle.c
  ../dist/libmpdclient/src/ierror.c
  ../dist/libmpdclient/src/iso8601.c
  ../dist/libmpdclient/src/kvlist.c
  ../dist/libmpdclient/src/list.c
  ../dist/libmpdclient/src/message.c
  ../dist/libmpdclient/src/mixer.c
  ../dist/libmpdclient/src/mount.c
  ../dist/libmpdclient/src/neighbor.c
  ../dist/libmpdclient/src/output.c
  ../dist/libmpdclient/src/parser.c
  ../dist/libmpdclient/src/partition.c
  ../dist/libmpdclient/src/password.c
  ../dist/libmpdclient/src/player.c
  ../dist/libmpdclient/src/playlist.c
  ../dist/libmpdclient/src/position.c
  ../dist/libmpdclient/src/queue.c
  ../dist/libmpdclient/src/quote.c
  ../dist/libmpdclient/src/replay_gain.c
  ../dist/libmpdclient/src/rdirectory.c
  ../dist/libmpdclient/src/readpicture.c
  ../dist/libmpdclient/src/recv.c
  ../dist/libmpdclient/src/replay_gain.c
  ../dist/libmpdclient/src/resolver.c
  ../dist/libmpdclient/src/response.c
  ../dist/libmpdclient/src/rplaylist.c
  ../dist/libmpdclient/src/run.c
  ../dist/libmpdclient/src/search.c
  ../dist/libmpdclient/src/send.c
  ../dist/libmpdclient/src/settings.c
  ../dist/libmpdclient/src/socket.c
  ../dist/libmpdclient/src/song.c
  ../dist/libmpdclient/src/stats.c
  ../dist/libmpdclient/src/status.c
  ../dist/libmpdclient/src/sticker.c
  ../dist/libmpdclient/src/sync.c
  ../dist/libmpdclient/src/tag.c
)

set(LIBASAN_FLAGS "-fsanitize=address \
  -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
  -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
  -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
  -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr \
  -static-libasan")

#set mongoose feature flags
set(MONGOOSE_FLAGS "-DMG_ENABLE_OPENSSL=1 -DMG_ENABLE_DIRECTORY_LISTING=1 -DMG_ENABLE_SSI=0")
if("${ENABLE_IPV6}" MATCHES "ON")
  message("IPv6 is enabled")
  set(MONGOOSE_FLAGS "${MONGOOSE_FLAGS} -DMG_ENABLE_IPV6=1")
else()
  message("IPv6 is disabled")
endif()
#set mjson feature flags
set(MJSON_FLAGS "-D MJSON_ENABLE_PRINT=0 -D MJSON_ENABLE_BASE64=0 -D MJSON_ENABLE_RPC=0 \
  -D MJSON_ENABLE_PRETTY=0 -D MJSON_ENABLE_MERGE=0")

#disabled for compatibility with tests
set(UTEST_FLAGS "-Wno-unused-function -Wno-redundant-decls")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
  -ffunction-sections -fdata-sections -Wall -Werror -Wvla -Winit-self -Wuninitialized \
  -Werror=array-bounds -Wextra -pedantic -Wunused-parameter -Wshadow -Wwrite-strings \
  -Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wnested-externs \
  -Wmissing-include-dirs -Wundef -Wformat-nonliteral -Wsign-compare \
  -Wno-stringop-overread -Wno-array-bounds \
  ${UTEST_FLAGS} ${LIBASAN_FLAGS} ${MONGOOSE_FLAGS} ${MJSON_FLAGS}")

# debug flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -Og")

#relax some compiler settings for dist sources
#compiler flags for mongoose
#-Wno-stringop-overflow: gcc 11.2.0 bug
#-Wno-stringop-overread: gcc 12.0.1 bug
set_property(SOURCE ../dist/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-conversion \
  -Wno-stringop-overflow")
#compiler flags for mjson
set_property(SOURCE ../dist/mjson/mjson.c PROPERTY COMPILE_FLAGS "-Wno-conversion")
#compiler flags for rax
#-Wno-use-after-free: gcc 12.0.1 error
set_property(SOURCE ../dist/rax/rax.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-conversion \
 -Wno-shadow -Wno-discarded-qualifiers -Wno-incompatible-pointer-types-discards-qualifiers -Wno-use-after-free")
#compiler flags for sds
#-Wno-stringop-overflow: gcc 11.2.0 bug
set_property(SOURCE ../dist/sds/sds.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-stringop-overflow -Wno-conversion")
#compiler flags for libmpdclient
set_property(SOURCE ${LIBMPDCLIENT_SOURCES} PROPERTY COMPILE_FLAGS "-Wno-conversion")

configure_file(../src/mympd_config_defs.h.in ${PROJECT_BINARY_DIR}/mympd_config_defs.h)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ../dist/libmpdclient/include)

include(CheckCSourceCompiles)

add_executable(test ${TEST_SOURCES} ${LIBMPDCLIENT_SOURCES})
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(test ${MATH_LIB})
target_link_libraries(test ${OPENSSL_LIBRARIES})
target_link_libraries(test ${PCRE2_LIBRARIES})
