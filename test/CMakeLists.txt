#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 3.4)

project (test C)

find_package(Threads REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O1 -Wall -Werror -Wvla -Winit-self -Wuninitialized -Werror=array-bounds \
    -ggdb -fsanitize=address \
    -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
    -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
    -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
    -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr \
    -static-libasan")

configure_file(mympd_config_defs.h.in ${PROJECT_BINARY_DIR}/mympd_config_defs.h)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ../dist/libmpdclient/include)

include(CheckCSourceCompiles)

set(SOURCES
  ../dist/mjson/mjson.c
  ../dist/rax/rax.c
  ../dist/sds/sds.c
  ../dist/tinymt/tinymt32.c
  ../src/lib/api.c
  ../src/lib/jsonrpc.c
  ../src/lib/list.c
  ../src/lib/log.c
  ../src/lib/lua_mympd_state.c
  ../src/lib/mimetype.c
  ../src/lib/mympd_queue.c
  ../src/lib/random.c
  ../src/lib/sds_extras.c
  ../src/lib/utility.c
  ../src/lib/validate.c
  ../src/mpd_shared.c
  ../src/mpd_shared/mpd_shared_tags.c
  main.c
  tests/test_jsonrpc.c
  tests/test_list.c
  tests/test_mimetype.c
  tests/test_mympd_queue.c
  tests/test_random.c
  tests/test_sds_extras.c
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

add_executable(test ${SOURCES} ${LIBMPDCLIENT_SOURCES})
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})
