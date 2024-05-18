# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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

if(MYMPD_ENABLE_LIBID3TAG)
  target_include_directories(mympd SYSTEM PRIVATE ${LIBID3TAG_INCLUDE_DIRS})
endif()
if(MYMPD_ENABLE_FLAC)
  target_include_directories(mympd SYSTEM PRIVATE ${FLAC_INCLUDE_DIRS})
endif()
if(MYMPD_ENABLE_LUA)
  target_include_directories(mympd SYSTEM PRIVATE ${LUA_INCLUDE_DIR})
endif()
if(MYMPD_ENABLE_MYGPIOD)
  if(MYMPD_ENABLE_MYGPIOD_STATIC)
    target_include_directories(mympd SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/dist/myGPIOd/libmygpio/include")
  else()
    target_include_directories(mympd SYSTEM PRIVATE ${LIBMYGPIO_INCLUDE_DIRS})
  endif()
endif()

target_sources(mympd
  PRIVATE
    main.c
    lib/album_cache.c
    lib/api.c
    lib/cache.c
    lib/cert.c
    lib/config.c
    lib/convert.c
    lib/covercache.c
    lib/datetime.c
    lib/env.c
    lib/event.c
    lib/fields.c
    lib/filehandler.c
    lib/handle_options.c
    lib/http_client.c
    lib/jsonrpc.c
    lib/last_played.c
    lib/list.c
    lib/log.c
    lib/m3u.c
    lib/mg_str_utils.c
    lib/mimetype.c
    lib/mpack.c
    lib/passwd.c
    lib/pin.c
    lib/msg_queue.c
    lib/mympd_state.c
    lib/random.c
    lib/rax_extras.c
    lib/sds_extras.c
    lib/smartpls.c
    lib/sticker.c
    lib/state_files.c
    lib/thread.c
    lib/timer.c
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
    mpd_client/random_select.c
    mpd_client/search.c
    mpd_client/search_local.c
    mpd_client/stickerdb.c
    mpd_client/shortcuts.c
    mpd_client/tags.c
    mpd_client/volume.c
    mpd_worker/mpd_worker.c
    mpd_worker/add_random.c
    mpd_worker/album_cache.c
    mpd_worker/api.c
    mpd_worker/jukebox.c
    mpd_worker/playlists.c
    mpd_worker/smartpls.c
    mpd_worker/state.c
    mpd_worker/song.c
    mympd_api/mympd_api.c
    mympd_api/albumart.c
    mympd_api/browse.c
    mympd_api/database.c
    mympd_api/extra_media.c
    mympd_api/filesystem.c
    mympd_api/home.c
    mympd_api/jukebox.c
    mympd_api/last_played.c
    mympd_api/lyrics.c
    mympd_api/mounts.c
    mympd_api/mympd_api_handler.c
    mympd_api/outputs.c
    mympd_api/partitions.c
    mympd_api/pictures.c
    mympd_api/playlists.c
    mympd_api/queue.c
    mympd_api/requests.c
    mympd_api/search.c
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
    web_server/folderart.c
    web_server/request_handler.c
    web_server/proxy.c
    web_server/radiobrowser.c
    web_server/sessions.c
    web_server/playlistart.c
    web_server/tagart.c
    web_server/utility.c
    web_server/webradiodb.c
)

if(MYMPD_ENABLE_LUA)
  target_sources(mympd
    PRIVATE
      scripts/scripts.c
      scripts/scripts_lua.c
      scripts/interface_http_client.c
      scripts/interface_mympd_api.c
      scripts/interface_util.c
      scripts/interface.c
      mympd_api/lua_mympd_state.c
      mympd_api/scripts.c
      mympd_api/scripts_vars.c
      web_server/scripts.c
  )
endif()

if(MYMPD_ENABLE_FLAC)
  target_sources(mympd
    PRIVATE
      mympd_api/lyrics_flac.c
      web_server/albumart_flac.c
  )
endif()

if(MYMPD_ENABLE_LIBID3TAG)
  target_sources(mympd
    PRIVATE
      mympd_api/lyrics_id3.c
      web_server/albumart_id3.c
  )
endif()

if(MYMPD_ENABLE_MYGPIOD)
  target_sources(mympd
    PRIVATE
      scripts/interface_mygpio.c
  )
endif()
