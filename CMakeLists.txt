# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

#myMPD is written in C
#supported compilers: gcc, clang
project (mympd C)

#update myMPD version here and run ./build.sh setversion to update
#the source files
set(CPACK_PACKAGE_VERSION_MAJOR "10")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

#minimal cmake version needed for correct GNUInstallDirs
cmake_minimum_required(VERSION 3.4)

message("Cmake version: ${CMAKE_VERSION}")

#available options
option(EMBEDDED_ASSETS "Embeds assets in binary, default ON" "ON")
option(ENABLE_FLAC "Enables flac usage, default ON" "ON")
option(ENABLE_IPV6 "Enables IPv6, default ON" "ON")
option(ENABLE_LUA "Enables lua usage, default ON" "ON")
option(ENABLE_LIBASAN "Enables build with libasan, default OFF" "OFF")
option(ENABLE_LIBID3TAG "Enables libid3tag usage, default ON" "ON")
option(ENABLE_SSL "Enables OpenSSL usage, default ON" "ON")
option(STRIP_BINARY "Enables stripping of the release binary, default ON" "ON")

#cmake modules
include(GNUInstallDirs)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)

#custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

#clang compatibility
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options("-Wno-unknown-warning-option")
endif()

message("Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")

#calculate paths
if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR "/${PROJECT_NAME}")
  set(SUBDIRLIB "/lib")
  set(SUBDIRCACHE "/cache")
else()
  #for install in /opt
  set(SUBDIR "")
  set(SUBDIRLIB "")
  set(SUBDIRCACHE "")
endif()

message("Executables in: ${CMAKE_INSTALL_FULL_BINDIR}")

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(WORK_DIR "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
else()
  set(WORK_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
endif()
message("Workdir: ${WORK_DIR}")

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(CACHE_DIR "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
else()
  set(CACHE_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
endif()
message("Cachedir: ${CACHE_DIR}")

#default is to deliver embedded assets for release
#and assets in htdocs for debug
if("${EMBEDDED_ASSETS}" MATCHES "ON")
  set(DOC_ROOT "${WORK_DIR}/empty")
  set(LUALIBS_PATH "")
  message("Embedding assets in binary")
else()
  set(DOC_ROOT "${PROJECT_SOURCE_DIR}/htdocs")
  set(LUALIBS_PATH "${PROJECT_SOURCE_DIR}/contrib/lualibs")
  message("Serving assets from filesystem")
endif()
message("Document root: ${DOC_ROOT}")

#set debug define for source
if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(DEBUG "OFF")
  set(NDEBUG "true")
else()
  set(DEBUG "ON")
endif()

#required dependencies
find_package(Threads REQUIRED)
find_package(PCRE2 REQUIRED)
find_library(MATH_LIB m REQUIRED)

#optional dependencies
if(NOT "${ENABLE_SSL}" MATCHES "OFF")
  message("Searching for openssl")
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
    if ("${OPENSSL_VERSION}" VERSION_GREATER_EQUAL "1.1.0")
      set(ENABLE_SSL "ON")
      set(MONGOOSE_SSL "-DMG_ENABLE_OPENSSL=1")
      include_directories(${OPENSSL_INCLUDE_DIR})
    else()
      message("OpenSSL is disabled because a version lower than 1.1.0 was found")
      set(ENABLE_SSL "OFF")
    endif()
  else()
    message("OpenSSL is disabled because it was not found")
    set(ENABLE_SSL "OFF")
  endif()
else()
  message("OpenSSL is disabled by user")
endif()

if(NOT "${ENABLE_LIBID3TAG}" MATCHES "OFF")
  message("Searching for libid3tag")
  find_package(LIBID3TAG)
  if(LIBID3TAG_FOUND)
    set(ENABLE_LIBID3TAG "ON")
    include_directories(${LIBID3TAG_INCLUDE_DIRS})
  else()
    message("Libid3tag is disabled because it was not found")
    set(ENABLE_LIBID3TAG "OFF")
  endif()
else("${ENABLE_LIBID3TAG}" MATCHES "OFF")
  message("Libid3tag is disabled by user")
endif()

if(NOT "${ENABLE_FLAC}" MATCHES "OFF")
  message("Searching for flac")
  find_package(FLAC)
  if(FLAC_FOUND)
    set(ENABLE_FLAC "ON")
    include_directories(${FLAC_INCLUDE_DIRS})
  else()
    message("Flac is disabled because it was not found")
    set(ENABLE_FLAC "OFF")
  endif()
else()
  message("Flac is disabled by user")
endif()

if(NOT "${ENABLE_LUA}" MATCHES "OFF")
  IF(EXISTS "/etc/alpine-release")
    set(ENV{LUA_DIR} "/usr/lib/lua5.4")
  endif()
  message("Searching for lua")
  find_package(Lua)
  if(LUA_FOUND)
    if("${LUA_VERSION_STRING}" VERSION_GREATER_EQUAL "5.3.0")
      set(ENABLE_LUA "ON")
      include_directories(${LUA_INCLUDE_DIR})
    else()
      message("Lua is disabled because a version lower than 5.3.0 was found")
      set(ENABLE_LUA "OFF")
    endif()
  else()
    message("Lua is disabled because it was not found")
    set(ENABLE_LUA "OFF")
  endif()
else()
  message("Lua is disabled by user")
endif()

#translation files
if("${EMBEDDED_ASSETS}" MATCHES "ON")
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/de-DE.json.gz")
    set(I18N_de_DE "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/en-US.json.gz")
    set(I18N_en_US "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/es-VE.json.gz")
    set(I18N_es_VE "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/fi-FI.json.gz")
    set(I18N_fi_FI "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/fr-FR.json.gz")
    set(I18N_fr_FR "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/it-IT.json.gz")
    set(I18N_it_IT "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/ja-JP.json.gz")
    set(I18N_ja_JP "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/ko-KR.json.gz")
    set(I18N_ko_KR "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/nl-NL.json.gz")
    set(I18N_nl_NL "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/zh-CN.json.gz")
    set(I18N_zh_CN "ON")
  endif()
endif()

#configure some files - version and path information
configure_file(src/compile_time.h.in ${PROJECT_BINARY_DIR}/compile_time.h)
configure_file(cmake/CopyConfig.cmake.in cmake/CopyConfig.cmake @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)

#set include dirs
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
include_directories(${PCRE2_INCLUDE_DIRS})

#Use the integrated libmympdclient for mpd communication
#myMPD is incompatible with the original libmpdclient
include_directories("dist/libmpdclient/include")

#check for enabling libasan
if("${ENABLE_LIBASAN}" MATCHES "ON")
  message("Compiling with libasan")
  set(LIBASAN_FLAGS "-fsanitize=address \
    -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
    -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
    -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
    -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr \
    -static-libasan")
else()
  set(LIBASAN_FLAGS "")
endif()

#set mongoose feature flags
set(MONGOOSE_FLAGS "${MONGOOSE_SSL} -DMG_ENABLE_DIRECTORY_LISTING=1 -DMG_ENABLE_SSI=0")
if("${ENABLE_IPV6}" MATCHES "ON")
  message("IPv6 is enabled")
  set(MONGOOSE_FLAGS "${MONGOOSE_FLAGS} -DMG_ENABLE_IPV6=1")
else()
  message("IPv6 is disabled")
endif()

#set mjson feature flags
set(MJSON_FLAGS "-D MJSON_ENABLE_PRINT=0 -D MJSON_ENABLE_BASE64=0 -D MJSON_ENABLE_RPC=0 \
  -D MJSON_ENABLE_PRETTY=0 -D MJSON_ENABLE_MERGE=0")

#set strict global compile flags
#-Wno-stringop-overread: gcc 12 error
#-Wno-array-bounds: gcc 12 error on armv7l
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
  -ffunction-sections -fdata-sections -Wall -Werror -Wvla -Winit-self -Wuninitialized \
	-Werror=array-bounds -Wextra -pedantic -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
	-Wmissing-include-dirs -Wundef -Wformat-nonliteral -Wsign-compare -Wconversion \
  -Wno-stringop-overread -Wno-array-bounds \
	${LIBASAN_FLAGS} ${MONGOOSE_FLAGS} ${MJSON_FLAGS}")

#check for supported compiler flags
foreach(FLAG IN ITEMS "-std=gnu17" "-fstack-clash-protection" "-fcf-protection" "-fno-plt")
  message("Checking for compiler flag ${FLAG}")
  unset(COMPILER_SUPPORTS_FLAG CACHE)
  unset(COMPILER_SUPPORTS_FLAG)
  CHECK_C_COMPILER_FLAG("${FLAG}" COMPILER_SUPPORTS_FLAG)
  if(COMPILER_SUPPORTS_FLAG MATCHES "1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
  endif()
endforeach()

#set debug/release flags
if(CMAKE_BUILD_TYPE MATCHES "RELEASE")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIE")
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro,-z,now,--gc-sections -pie")
  if(STRIP_BINARY MATCHES "ON")
    message("Generating stripped binary")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
  else()
    message("Generating binary with debug symbols")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
  endif()
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -Og")
  set(CMAKE_EXE_LINKER_FLAGS "")
endif()

#set source files
set(MYMPD_SOURCES
  dist/mjson/mjson.c
  dist/mongoose/mongoose.c
  dist/rax/rax.c
  dist/sds/sds.c
  dist/tinymt/tinymt32.c
  src/lib/album_cache.c
  src/lib/api.c
  src/lib/config.c
  src/lib/covercache.c
  src/lib/filehandler.c
  src/lib/handle_options.c
  src/lib/http_client.c
  src/lib/jsonrpc.c
  src/lib/list.c
  src/lib/log.c
  src/lib/lua_mympd_state.c
  src/lib/m3u.c
  src/lib/mimetype.c
  src/lib/pin.c
  src/lib/msg_queue.c
  src/lib/mympd_state.c
  src/lib/random.c
  src/lib/rax_extras.c
  src/lib/sds_extras.c
  src/lib/smartpls.c
  src/lib/state_files.c
  src/lib/sticker_cache.c
  src/lib/utility.c
  src/lib/validate.c
  src/main.c
  src/mpd_client/autoconf.c
  src/mpd_client/connection.c
  src/mpd_client/errorhandler.c
  src/mpd_client/features.c
  src/mpd_client/idle.c
  src/mpd_client/jukebox.c
  src/mpd_client/partitions.c
  src/mpd_client/playlists.c
  src/mpd_client/search.c
  src/mpd_client/search_local.c
  src/mpd_client/tags.c
  src/mpd_client/volume.c
  src/mpd_worker/mpd_worker.c
  src/mpd_worker/api.c
  src/mpd_worker/cache.c
  src/mpd_worker/smartpls.c
  src/mpd_worker/state.c
  src/mympd_api/mympd_api.c
  src/mympd_api/albumart.c
  src/mympd_api/browse.c
  src/mympd_api/extra_media.c
  src/mympd_api/filesystem.c
  src/mympd_api/home.c
  src/mympd_api/last_played.c
  src/mympd_api/lyrics.c
  src/mympd_api/mounts.c
  src/mympd_api/mympd_api_handler.c
  src/mympd_api/outputs.c
  src/mympd_api/partitions.c
  src/mympd_api/pictures.c
  src/mympd_api/playlists.c
  src/mympd_api/queue.c
  src/mympd_api/scripts.c
  src/mympd_api/settings.c
  src/mympd_api/smartpls.c
  src/mympd_api/song.c
  src/mympd_api/stats.c
  src/mympd_api/status.c
  src/mympd_api/sticker.c
  src/mympd_api/timer.c
  src/mympd_api/timer_handlers.c
  src/mympd_api/trigger.c
  src/mympd_api/volume.c
  src/mympd_api/webradios.c
  src/web_server/web_server.c
  src/web_server/albumart.c
  src/web_server/request_handler.c
  src/web_server/proxy.c
  src/web_server/radiobrowser.c
  src/web_server/sessions.c
  src/web_server/tagart.c
  src/web_server/utility.c
  src/web_server/webradiodb.c
)

if("${ENABLE_SSL}" MATCHES "ON")
  set(CERT_SOURCES
      src/lib/cert.c
  )
endif()

set(LIBMPDCLIENT_SOURCES
  dist/libmpdclient/src/albumart.c
  dist/libmpdclient/src/async.c
  dist/libmpdclient/src/audio_format.c
  dist/libmpdclient/src/binary.c
  dist/libmpdclient/src/capabilities.c
  dist/libmpdclient/src/cmessage.c
  dist/libmpdclient/src/cmount.c
  dist/libmpdclient/src/cneighbor.c
  dist/libmpdclient/src/connection.c
  dist/libmpdclient/src/coutput.c
  dist/libmpdclient/src/cpartition.c
  dist/libmpdclient/src/cplaylist.c
  dist/libmpdclient/src/cstats.c
  dist/libmpdclient/src/cstatus.c
  dist/libmpdclient/src/database.c
  dist/libmpdclient/src/directory.c
  dist/libmpdclient/src/entity.c
  dist/libmpdclient/src/error.c
  dist/libmpdclient/src/fd_util.c
  dist/libmpdclient/src/fingerprint.c
  dist/libmpdclient/src/idle.c
  dist/libmpdclient/src/ierror.c
  dist/libmpdclient/src/iso8601.c
  dist/libmpdclient/src/kvlist.c
  dist/libmpdclient/src/list.c
  dist/libmpdclient/src/message.c
  dist/libmpdclient/src/mixer.c
  dist/libmpdclient/src/mount.c
  dist/libmpdclient/src/neighbor.c
  dist/libmpdclient/src/output.c
  dist/libmpdclient/src/parser.c
  dist/libmpdclient/src/partition.c
  dist/libmpdclient/src/password.c
  dist/libmpdclient/src/player.c
  dist/libmpdclient/src/playlist.c
  dist/libmpdclient/src/position.c
  dist/libmpdclient/src/queue.c
  dist/libmpdclient/src/quote.c
  dist/libmpdclient/src/replay_gain.c
  dist/libmpdclient/src/rdirectory.c
  dist/libmpdclient/src/readpicture.c
  dist/libmpdclient/src/recv.c
  dist/libmpdclient/src/replay_gain.c
  dist/libmpdclient/src/resolver.c
  dist/libmpdclient/src/response.c
  dist/libmpdclient/src/rplaylist.c
  dist/libmpdclient/src/run.c
  dist/libmpdclient/src/search.c
  dist/libmpdclient/src/send.c
  dist/libmpdclient/src/settings.c
  dist/libmpdclient/src/socket.c
  dist/libmpdclient/src/song.c
  dist/libmpdclient/src/stats.c
  dist/libmpdclient/src/status.c
  dist/libmpdclient/src/sticker.c
  dist/libmpdclient/src/sync.c
  dist/libmpdclient/src/tag.c
)

#relax some compiler settings for dist sources

#compiler flags for mongoose
set_property(SOURCE dist/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-conversion")

#compiler flags for mjson
#clang15 warning
set_property(SOURCE dist/mjson/mjson.c PROPERTY COMPILE_FLAGS "-Wno-unused-but-set-variable")

#compiler flags for rax
#-Wno-use-after-free: gcc 12.0.1 error
set_property(SOURCE dist/rax/rax.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-conversion \
  -Wno-shadow -Wno-discarded-qualifiers -Wno-incompatible-pointer-types-discards-qualifiers -Wno-use-after-free")

#compiler flags for sds
set_property(SOURCE dist/sds/sds.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral -Wno-conversion")

#compiler flags for libmpdclient
set_property(SOURCE ${LIBMPDCLIENT_SOURCES} PROPERTY COMPILE_FLAGS "-Wno-conversion")

#compile the executeable
add_executable(mympd ${MYMPD_SOURCES} ${LIBMPDCLIENT_SOURCES} ${CERT_SOURCES})

#link required dependencies
target_link_libraries(mympd ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(mympd ${MATH_LIB})
target_link_libraries(mympd ${PCRE2_LIBRARIES})

#link optional dependencies
if(OPENSSL_FOUND)
  target_link_libraries(mympd ${OPENSSL_LIBRARIES})
endif()
if(LIBID3TAG_FOUND)
  target_link_libraries(mympd ${LIBID3TAG_LIBRARIES})
endif()
if(FLAC_FOUND)
  target_link_libraries(mympd ${FLAC_LIBRARIES})
endif()
if(LUA_FOUND)
  target_link_libraries(mympd ${LUA_LIBRARIES})
endif()

#install
install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)

#sub projects
add_subdirectory("cli_tools")
