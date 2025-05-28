# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
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
    lib/api.c
    lib/cacertstore.c
    lib/cache/cache_disk_images.c
    lib/cache/cache_disk_lyrics.c
    lib/cache/cache_disk.c
    lib/cache/cache_rax_album.c
    lib/cache/cache_rax.c
    lib/cert.c
    lib/config.c
    lib/convert.c
    lib/datetime.c
    lib/env.c
    lib/event.c
    lib/fields.c
    lib/filehandler.c
    lib/handle_options.c
    lib/http_client.c
    lib/http_client_cache.c
    lib/json/json_print.c
    lib/json/json_query.c
    lib/json/json_rpc.c
    lib/last_played.c
    lib/list.c
    lib/log.c
    lib/mimetype.c
    lib/mpack.c
    lib/passwd.c
    lib/pin.c
    lib/msg_queue.c
    lib/mympd_state.c
    lib/random.c
    lib/rax_extras.c
    lib/search.c
    lib/sds_extras.c
    lib/signal.c
    lib/smartpls.c
    lib/sticker.c
    lib/state_files.c
    lib/thread.c
    lib/timer.c
    lib/utility.c
    lib/validate.c
    lib/webradio.c
    mympd_client/autoconf.c
    mympd_client/connection.c
    mympd_client/errorhandler.c
    mympd_client/features.c
    mympd_client/idle.c
    mympd_client/jukebox.c
    mympd_client/partitions.c
    mympd_client/playlists.c
    mympd_client/queue.c
    mympd_client/presets.c
    mympd_client/random_select.c
    mympd_client/search.c
    mympd_client/stickerdb.c
    mympd_client/shortcuts.c
    mympd_client/tags.c
    mympd_client/volume.c
    mympd_worker/mympd_worker.c
    mympd_worker/album_cache.c
    mympd_worker/api.c
    mympd_worker/jukebox.c
    mympd_worker/playlists.c
    mympd_worker/random_select.c
    mympd_worker/smartpls.c
    mympd_worker/state.c
    mympd_worker/song.c
    mympd_worker/webradiodb.c
    mympd_api/mympd_api.c
    mympd_api/albumart.c
    mympd_api/albums.c
    mympd_api/channel.c
    mympd_api/database.c
    mympd_api/extra_media.c
    mympd_api/filesystem.c
    mympd_api/folderart.c
    mympd_api/home.c
    mympd_api/jukebox.c
    mympd_api/last_played.c
    mympd_api/lyrics.c
    mympd_api/mounts.c
    mympd_api/mympd_api_handler.c
    mympd_api/outputs.c
    mympd_api/partitions.c
    mympd_api/pictures.c
    mympd_api/playlistart.c
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
    mympd_api/tagart.c
    mympd_api/tags.c
    mympd_api/timer.c
    mympd_api/timer_handlers.c
    mympd_api/trigger.c
    mympd_api/volume.c
    mympd_api/webradio.c
    mympd_api/webradio_favorites.c
    webserver/webserver.c
    webserver/albumart.c
    webserver/folderart.c
    webserver/mg_user_data.c
    webserver/request_handler.c
    webserver/placeholder.c
    webserver/proxy.c
    webserver/sessions.c
    webserver/playlistart.c
    webserver/tagart.c
    webserver/utility.c
    webserver/webradio.c
)

if(MYMPD_ENABLE_LUA)
  target_sources(mympd
    PRIVATE
      mympd_api/lua_mympd_state.c
      scripts/api_handler.c
      scripts/api_scripts.c
      scripts/api_tmp.c
      scripts/api_vars.c
      scripts/events.c
      scripts/interface_caches.c
      scripts/interface_http.c
      scripts/interface_mympd_api.c
      scripts/interface_util.c
      scripts/interface.c
      scripts/scripts_lua.c
      scripts/scripts_worker.c
      scripts/scripts.c
      scripts/util.c
      scripts/verify.c
      webserver/scripts.c
  )
endif()

if(MYMPD_ENABLE_FLAC)
  target_sources(mympd
    PRIVATE
      mympd_api/lyrics_flac.c
      webserver/albumart_flac.c
  )
endif()

if(MYMPD_ENABLE_LIBID3TAG)
  target_sources(mympd
    PRIVATE
      mympd_api/lyrics_id3.c
      webserver/albumart_id3.c
  )
endif()

if(MYMPD_ENABLE_MYGPIOD)
  target_sources(mympd
    PRIVATE
      scripts/interface_mygpio.c
  )
endif()
