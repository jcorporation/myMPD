# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

set(MYMPD_BUILD_DIR "${PROJECT_BINARY_DIR}")
configure_file(utility.h.in "${PROJECT_BINARY_DIR}/test/utility.h")

set(TEST_SOURCES
  main.c
  utility.c
  ../src/lib/api.c
  ../src/lib/cacertstore.c
  ../src/lib/cache/cache_disk_lyrics.c
  ../src/lib/cache/cache_rax_album.c
  ../src/lib/cache/cache_rax.c
  ../src/lib/cert.c
  ../src/lib/config.c
  ../src/lib/convert.c
  ../src/lib/datetime.c
  ../src/lib/event.c
  ../src/lib/env.c
  ../src/lib/fields.c
  ../src/lib/filehandler.c
  ../src/lib/http_client.c
  ../src/lib/http_client_cache.c
  ../src/lib/json/json_print.c
  ../src/lib/json/json_query.c
  ../src/lib/json/json_rpc.c
  ../src/lib/last_played.c
  ../src/lib/list.c
  ../src/lib/log.c
  ../src/lib/mimetype.c
  ../src/lib/mpack.c
  ../src/lib/msg_queue.c
  ../src/lib/mympd_state.c
  ../src/lib/passwd.c
  ../src/lib/random.c
  ../src/lib/rax_extras.c
  ../src/lib/sds_extras.c
  ../src/lib/search.c
  ../src/lib/smartpls.c
  ../src/lib/state_files.c
  ../src/lib/sticker.c
  ../src/lib/timer.c
  ../src/lib/utility.c
  ../src/lib/validate.c
  ../src/lib/webradio.c
  ../src/mympd_client/connection.c
  ../src/mympd_client/errorhandler.c
  ../src/mympd_client/features.c
  ../src/mympd_client/jukebox.c
  ../src/mympd_client/presets.c
  ../src/mympd_client/queue.c
  ../src/mympd_client/playlists.c
  ../src/mympd_client/random_select.c
  ../src/mympd_client/search.c
  ../src/mympd_client/shortcuts.c
  ../src/mympd_client/stickerdb.c
  ../src/mympd_client/tags.c
  ../src/mympd_client/jukebox.c
  ../src/mympd_client/volume.c
  ../src/mympd_api/extra_media.c
  ../src/mympd_api/home.c
  ../src/mympd_api/last_played.c
  ../src/mympd_api/jukebox.c
  ../src/mympd_api/lua_mympd_state.c
  ../src/mympd_api/lyrics.c
  ../src/mympd_api/requests.c
  ../src/mympd_api/settings.c
  ../src/mympd_api/status.c
  ../src/mympd_api/sticker.c
  ../src/mympd_api/timer.c
  ../src/mympd_api/timer_handlers.c
  ../src/mympd_api/trigger.c
  ../src/mympd_api/queue.c
  ../src/mympd_api/webradio.c
  ../src/scripts/events.c
  ../src/webserver/mg_user_data.c
  tests/test_album_cache.c
  tests/test_api.c
  tests/test_cert.c
  tests/test_convert.c
  tests/test_datetime.c
  tests/test_env.c
  tests/test_filehandler.c
  tests/test_http_client.c
  tests/test_http_client_cache.c
  tests/test_jsonrpc.c
  tests/test_list.c
  tests/test_mimetype.c
  tests/test_mympd_queue.c
  tests/test_mympd_state.c
  tests/test_radix_sort.c
  tests/test_random.c
  tests/test_sds_extras.c
  tests/test_search.c
  tests/test_state_files.c
  tests/test_tags.c
  tests/test_timer.c
  tests/test_utility.c
  tests/test_validate.c
)

if(LIBID3TAG_FOUND)
  set(TEST_SOURCES_LIBID3TAG
    ../src/mympd_api/lyrics_id3.c
    tests/test_lyrics_id3.c
  )
endif()
if(FLAC_FOUND)
  set(TEST_SOURCES_FLAC
  ../src/mympd_api/lyrics_flac.c
    tests/test_lyrics_flac.c
  )
endif()

add_executable(unit_test
  ${TEST_SOURCES}
  ${TEST_SOURCES_LIBID3TAG}
  ${TEST_SOURCES_FLAC}
)

target_include_directories(unit_test
  PRIVATE 
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/test
    ${PROJECT_SOURCE_DIR}
)

target_compile_options(unit_test
  PRIVATE
    "-Wno-unused-function"
    "-Wno-redundant-decls"
    "-DMG_MAX_HTTP_HEADERS=50"
)

target_link_libraries(unit_test
  mympdclient
  mpack
  mongoose
  rax
  sds
  ${CMAKE_THREAD_LIBS_INIT}
  ${MATH_LIB}
  ${OPENSSL_LIBRARIES}
  ${PCRE2_LIBRARIES}
)

# link optional dependencies
if(LIBID3TAG_FOUND)
  target_link_libraries(unit_test ${LIBID3TAG_LIBRARIES})
endif()
if(FLAC_FOUND)
  target_link_libraries(unit_test ${FLAC_LIBRARIES})
endif()

add_custom_command(TARGET unit_test PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${CMAKE_SOURCE_DIR}/test/testfiles ${PROJECT_BINARY_DIR}/testfiles)

# define tests
list(APPEND test_categories
  "album_cache"
  "api"
  "cert"
  "convert"
  "datetime"
  "env"
  "filehandler"
  "http_client"
  "jsonrpc"
  "list"
  "m3u"
  "mimetype"
  "mympd_queue"
  "mympd_state"
  "passwd"
  "radix_sort"
  "random"
  "sds_extras"
  "search_local"
  "state_files"
  "tags"
  "timer"
  "utility"
  "validate"
)

if(LIBID3TAG_FOUND)
  list(APPEND test_categories "lyrics_id3")
endif()
if(FLAC_FOUND)
  list(APPEND test_categories "lyrics_flac")
endif()

foreach(CAT IN LISTS test_categories)
  add_test(NAME "test_${CAT}" COMMAND "unit_test" "--filter=${CAT}.*")
endforeach()
