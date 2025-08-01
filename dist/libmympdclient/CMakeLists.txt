# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(mympdclient "")

target_include_directories(mympdclient PUBLIC include)

if(CMAKE_BUILD_TYPE MATCHES "(Release|RelWithDebInfo|MinSizeRel)")
  target_compile_options(mympdclient
    PRIVATE
      "-DNDEBUG"
  )
endif()

target_sources(mympdclient
  PRIVATE
    src/albumart.c
    src/async.c
    src/audio_format.c
    src/binary.c
    src/capabilities.c
    src/cmessage.c
    src/cmount.c
    src/cneighbor.c
    src/connection.c
    src/coutput.c
    src/cpartition.c
    src/cplaylist.c
    src/cstats.c
    src/cstatus.c
    src/database.c
    src/directory.c
    src/entity.c
    src/error.c
    src/fd_util.c
    src/feature.c
    src/fingerprint.c
    src/idle.c
    src/ierror.c
    src/iso8601.c
    src/kvlist.c
    src/list.c
    src/message.c
    src/mixer.c
    src/mount.c
    src/neighbor.c
    src/output.c
    src/parser.c
    src/partition.c
    src/password.c
    src/player.c
    src/playlist.c
    src/position.c
    src/queue.c
    src/quote.c
    src/replay_gain.c
    src/rdirectory.c
    src/readpicture.c
    src/recv.c
    src/replay_gain.c
    src/resolver.c
    src/response.c
    src/rplaylist.c
    src/run.c
    src/request.c
    src/search.c
    src/send.c
    src/settings.c
    src/socket.c
    src/song.c
    src/stats.c
    src/status.c
    src/sticker.c
    src/sync.c
    src/tag.c
)
