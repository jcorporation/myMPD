#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 3.4)

project(test C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../cmake/")

find_package(Threads REQUIRED)
find_library(MATH_LIB m REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PCRE2 REQUIRED)
find_package(LIBID3TAG REQUIRED)
find_package(FLAC REQUIRED)

set(TEST_SOURCES
  ../dist/mjson/mjson.c
  ../dist/mongoose/mongoose.c
  ../dist/rax/rax.c
  ../dist/sds/sds.c
  ../dist/tinymt/tinymt32.c
  ../src/lib/album_cache.c
  ../src/lib/api.c
  ../src/lib/cert.c
  ../src/lib/filehandler.c
  ../src/lib/http_client.c
  ../src/lib/jsonrpc.c
  ../src/lib/list.c
  ../src/lib/log.c
  ../src/lib/lua_mympd_state.c
  ../src/lib/m3u.c
  ../src/lib/mimetype.c
  ../src/lib/msg_queue.c
  ../src/lib/mympd_state.c
  ../src/lib/random.c
  ../src/lib/sds_extras.c
  ../src/lib/state_files.c
  ../src/lib/sticker_cache.c
  ../src/lib/utility.c
  ../src/lib/validate.c
  ../src/mpd_client/errorhandler.c
  ../src/mpd_client/presets.c
  ../src/mpd_client/search.c
  ../src/mpd_client/search_local.c
  ../src/mpd_client/tags.c
  ../src/mpd_client/jukebox.c
  ../src/mpd_client/volume.c
  ../src/mympd_api/extra_media.c
  ../src/mympd_api/home.c
  ../src/mympd_api/last_played.c
  ../src/mympd_api/lyrics.c
  ../src/mympd_api/status.c
  ../src/mympd_api/sticker.c
  ../src/mympd_api/timer.c
  ../src/mympd_api/timer_handlers.c
  ../src/mympd_api/trigger.c
  ../src/mympd_api/queue.c
  ../src/mympd_api/webradios.c
  main.c
  tests/test_api.c
  tests/test_cert.c
  tests/test_http_client.c
  tests/test_jsonrpc.c
  tests/test_list.c
  tests/test_lyrics.c
  tests/test_m3u.c
  tests/test_mpd_client_tags.c
  tests/test_mimetype.c
  tests/test_mympd_queue.c
  tests/test_radix_sort.c
  tests/test_random.c
  tests/test_sds_extras.c
  tests/test_state_files.c
  tests/test_timer.c
  tests/test_utility.c
  tests/test_validate.c
)

set(LIBMPDCLIENT_SOURCES
  ../dist/libmympdclient/src/albumart.c
  ../dist/libmympdclient/src/async.c
  ../dist/libmympdclient/src/audio_format.c
  ../dist/libmympdclient/src/binary.c
  ../dist/libmympdclient/src/capabilities.c
  ../dist/libmympdclient/src/cmessage.c
  ../dist/libmympdclient/src/cmount.c
  ../dist/libmympdclient/src/cneighbor.c
  ../dist/libmympdclient/src/connection.c
  ../dist/libmympdclient/src/coutput.c
  ../dist/libmympdclient/src/cpartition.c
  ../dist/libmympdclient/src/cplaylist.c
  ../dist/libmympdclient/src/cstats.c
  ../dist/libmympdclient/src/cstatus.c
  ../dist/libmympdclient/src/database.c
  ../dist/libmympdclient/src/directory.c
  ../dist/libmympdclient/src/entity.c
  ../dist/libmympdclient/src/error.c
  ../dist/libmympdclient/src/fd_util.c
  ../dist/libmympdclient/src/fingerprint.c
  ../dist/libmympdclient/src/idle.c
  ../dist/libmympdclient/src/ierror.c
  ../dist/libmympdclient/src/iso8601.c
  ../dist/libmympdclient/src/kvlist.c
  ../dist/libmympdclient/src/list.c
  ../dist/libmympdclient/src/message.c
  ../dist/libmympdclient/src/mixer.c
  ../dist/libmympdclient/src/mount.c
  ../dist/libmympdclient/src/neighbor.c
  ../dist/libmympdclient/src/output.c
  ../dist/libmympdclient/src/parser.c
  ../dist/libmympdclient/src/partition.c
  ../dist/libmympdclient/src/password.c
  ../dist/libmympdclient/src/player.c
  ../dist/libmympdclient/src/playlist.c
  ../dist/libmympdclient/src/position.c
  ../dist/libmympdclient/src/queue.c
  ../dist/libmympdclient/src/quote.c
  ../dist/libmympdclient/src/replay_gain.c
  ../dist/libmympdclient/src/rdirectory.c
  ../dist/libmympdclient/src/readpicture.c
  ../dist/libmympdclient/src/recv.c
  ../dist/libmympdclient/src/replay_gain.c
  ../dist/libmympdclient/src/resolver.c
  ../dist/libmympdclient/src/response.c
  ../dist/libmympdclient/src/rplaylist.c
  ../dist/libmympdclient/src/run.c
  ../dist/libmympdclient/src/search.c
  ../dist/libmympdclient/src/send.c
  ../dist/libmympdclient/src/settings.c
  ../dist/libmympdclient/src/socket.c
  ../dist/libmympdclient/src/song.c
  ../dist/libmympdclient/src/stats.c
  ../dist/libmympdclient/src/status.c
  ../dist/libmympdclient/src/sticker.c
  ../dist/libmympdclient/src/sync.c
  ../dist/libmympdclient/src/tag.c
)

set(LIBASAN_FLAGS "-fsanitize=address \
  -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
  -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
  -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
  -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr \
  -static-libasan")

#set mongoose feature flags
set(MONGOOSE_FLAGS "-DMG_ENABLE_OPENSSL=1 -DMG_ENABLE_DIRECTORY_LISTING=1 -DMG_ENABLE_SSI=0")
if(ENABLE_IPV6)
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
#compiler flags for libmympdclient
set_property(SOURCE ${LIBMPDCLIENT_SOURCES} PROPERTY COMPILE_FLAGS "-Wno-conversion")

set(MYMPD_ENABLE_FLAC "ON")
set(MYMPD_ENABLE_LIBID3TAG "ON")
configure_file(../src/compile_time.h.in ${PROJECT_BINARY_DIR}/compile_time.h)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/.. ../dist/libmympdclient/include)

include(CheckCSourceCompiles)

add_executable(test ${TEST_SOURCES} ${LIBMPDCLIENT_SOURCES})
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(test ${MATH_LIB})
target_link_libraries(test ${OPENSSL_LIBRARIES})
target_link_libraries(test ${PCRE2_LIBRARIES})
target_link_libraries(test ${LIBID3TAG_LIBRARIES})
target_link_libraries(test ${FLAC_LIBRARIES})
