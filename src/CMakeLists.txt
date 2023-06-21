# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_executable(mympd "")

if(CMAKE_BUILD_TYPE MATCHES "(Release|Debug)")
  target_compile_options(mympd
    PRIVATE
      "-Wconversion"
      "-Wformat=2"
  )
endif()

target_include_directories(mympd
  PRIVATE 
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_include_directories(mympd SYSTEM PRIVATE ${PCRE2_INCLUDE_DIRS})

if(MYMPD_ENABLE_LUA)
  target_include_directories(mympd SYSTEM PRIVATE ${LUA_INCLUDE_DIR})
endif()

if(MYMPD_ENABLE_FLAC)
  target_include_directories(mympd SYSTEM PRIVATE ${FLAC_INCLUDE_DIRS})
endif()

if(MYMPD_ENABLE_LIBID3TAG)
  target_include_directories(mympd SYSTEM PRIVATE ${LIBID3TAG_INCLUDE_DIRS})
endif()

target_sources(mympd PRIVATE
  main.c
  lib/album_cache.c
  lib/api.c
  lib/cert.c
  lib/config.c
  lib/covercache.c
  lib/env.c
  lib/filehandler.c
  lib/handle_options.c
  lib/http_client.c
  lib/jsonrpc.c
  lib/list.c
  lib/log.c
  lib/lua_mympd_state.c
  lib/m3u.c
  lib/mimetype.c
  lib/pin.c
  lib/msg_queue.c
  lib/mympd_state.c
  lib/random.c
  lib/rax_extras.c
  lib/sds_extras.c
  lib/smartpls.c
  lib/state_files.c
  lib/sticker_cache.c
  lib/utility.c
  lib/validate.c
  mpd_client/autoconf.c
  mpd_client/connection.c
  mpd_client/errorhandler.c
  mpd_client/features.c
  mpd_client/idle.c
  mpd_client/jukebox.c
  mpd_client/partitions.c
  mpd_client/playlists.c
  mpd_client/queue.c
  mpd_client/presets.c
  mpd_client/search.c
  mpd_client/search_local.c
  mpd_client/shortcuts.c
  mpd_client/tags.c
  mpd_client/volume.c
  mpd_worker/mpd_worker.c
  mpd_worker/api.c
  mpd_worker/cache.c
  mpd_worker/smartpls.c
  mpd_worker/state.c
  mympd_api/mympd_api.c
  mympd_api/albumart.c
  mympd_api/browse.c
  mympd_api/extra_media.c
  mympd_api/filesystem.c
  mympd_api/home.c
  mympd_api/last_played.c
  mympd_api/lyrics.c
  mympd_api/mounts.c
  mympd_api/mympd_api_handler.c
  mympd_api/outputs.c
  mympd_api/partitions.c
  mympd_api/pictures.c
  mympd_api/playlists.c
  mympd_api/queue.c
  mympd_api/scripts.c
  mympd_api/settings.c
  mympd_api/smartpls.c
  mympd_api/song.c
  mympd_api/stats.c
  mympd_api/status.c
  mympd_api/sticker.c
  mympd_api/timer.c
  mympd_api/timer_handlers.c
  mympd_api/trigger.c
  mympd_api/volume.c
  mympd_api/webradios.c
  web_server/web_server.c
  web_server/albumart.c
  web_server/request_handler.c
  web_server/proxy.c
  web_server/radiobrowser.c
  web_server/sessions.c
  web_server/tagart.c
  web_server/utility.c
  web_server/webradiodb.c
)
