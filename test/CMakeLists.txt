#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ../dist/src/libmpdclient/include)

include(CheckCSourceCompiles)

set(SOURCES
  ../src/lib/jsonrpc.c
  ../src/lib/list.c
  ../src/lib/mem.c
  ../src/lib/mimetype.c
  ../src/lib/mympd_queue.c
  ../src/lib/log.c
  ../src/lib/random.c
  ../src/lib/sds_extras.c
  ../src/lib/validate.c
  ../dist/src/mjson/mjson.c
  ../dist/src/sds/sds.c
  ../dist/src/utf8decode/utf8decode.c
  main.c
  tests/test_jsonrpc.c
  tests/test_list.c
  tests/test_mimetype.c
  tests/test_mympd_queue.c
  tests/test_sds_extras.c
  tests/test_validate.c
)

set(LIBMPDCLIENT_SOURCES
  ../dist/src/libmpdclient/src/albumart.c
  ../dist/src/libmpdclient/src/async.c
  ../dist/src/libmpdclient/src/audio_format.c
  ../dist/src/libmpdclient/src/binary.c
  ../dist/src/libmpdclient/src/capabilities.c
  ../dist/src/libmpdclient/src/cmessage.c
  ../dist/src/libmpdclient/src/cmount.c
  ../dist/src/libmpdclient/src/cneighbor.c
  ../dist/src/libmpdclient/src/connection.c
  ../dist/src/libmpdclient/src/coutput.c
  ../dist/src/libmpdclient/src/cpartition.c
  ../dist/src/libmpdclient/src/cplaylist.c
  ../dist/src/libmpdclient/src/cstats.c
  ../dist/src/libmpdclient/src/cstatus.c
  ../dist/src/libmpdclient/src/database.c
  ../dist/src/libmpdclient/src/directory.c
  ../dist/src/libmpdclient/src/entity.c
  ../dist/src/libmpdclient/src/error.c
  ../dist/src/libmpdclient/src/fd_util.c
  ../dist/src/libmpdclient/src/fingerprint.c
  ../dist/src/libmpdclient/src/idle.c
  ../dist/src/libmpdclient/src/ierror.c
  ../dist/src/libmpdclient/src/iso8601.c
  ../dist/src/libmpdclient/src/kvlist.c
  ../dist/src/libmpdclient/src/list.c
  ../dist/src/libmpdclient/src/message.c
  ../dist/src/libmpdclient/src/mixer.c
  ../dist/src/libmpdclient/src/mount.c
  ../dist/src/libmpdclient/src/neighbor.c
  ../dist/src/libmpdclient/src/output.c
  ../dist/src/libmpdclient/src/parser.c
  ../dist/src/libmpdclient/src/partition.c
  ../dist/src/libmpdclient/src/password.c
  ../dist/src/libmpdclient/src/player.c
  ../dist/src/libmpdclient/src/playlist.c
  ../dist/src/libmpdclient/src/queue.c
  ../dist/src/libmpdclient/src/quote.c
  ../dist/src/libmpdclient/src/replay_gain.c
  ../dist/src/libmpdclient/src/rdirectory.c
  ../dist/src/libmpdclient/src/readpicture.c
  ../dist/src/libmpdclient/src/recv.c
  ../dist/src/libmpdclient/src/replay_gain.c
  ../dist/src/libmpdclient/src/resolver.c
  ../dist/src/libmpdclient/src/response.c
  ../dist/src/libmpdclient/src/rplaylist.c
  ../dist/src/libmpdclient/src/run.c
  ../dist/src/libmpdclient/src/search.c
  ../dist/src/libmpdclient/src/send.c
  ../dist/src/libmpdclient/src/settings.c
  ../dist/src/libmpdclient/src/socket.c
  ../dist/src/libmpdclient/src/song.c
  ../dist/src/libmpdclient/src/stats.c
  ../dist/src/libmpdclient/src/status.c
  ../dist/src/libmpdclient/src/sticker.c
  ../dist/src/libmpdclient/src/sync.c
  ../dist/src/libmpdclient/src/tag.c
)

add_executable(test ${SOURCES} ${LIBMPDCLIENT_SOURCES})
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})
