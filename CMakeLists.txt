# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# minimal cmake version needed for new option handling
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0003 NEW)

# myMPD is written in C
# supported compilers: gcc, clang
project(mympd C)

# update myMPD version here and run ./build.sh setversion to update
# the source files
set(CPACK_PACKAGE_VERSION_MAJOR "10")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

# output binaries in bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message("Cmake version: ${CMAKE_VERSION}")
message("Cmake src dir: ${PROJECT_SOURCE_DIR}")
message("Cmake build dir: ${CMAKE_CURRENT_BINARY_DIR}")
message("Cmake build type: ${CMAKE_BUILD_TYPE}")
message("Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")

# default is to deliver the assets from htdocs for Debug
# and embedded assets for all other targets
if(NOT DEFINED MYMPD_EMBEDDED_ASSETS)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MYMPD_EMBEDDED_ASSETS "OFF")
  else()
    set(MYMPD_EMBEDDED_ASSETS "ON")
  endif()
endif()

# available options
option(MYMPD_EMBEDDED_ASSETS "Embeds assets in binary, default ON" "ON")
option(MYMPD_ENABLE_FLAC "Enables flac usage, default ON" "ON")
option(MYMPD_ENABLE_IPV6 "Enables IPv6, default ON" "ON")
option(MYMPD_ENABLE_LUA "Enables lua usage, default ON" "ON")
option(MYMPD_ENABLE_LIBASAN "Enables build with libasan, default OFF" "OFF")
option(MYMPD_ENABLE_LIBID3TAG "Enables libid3tag usage, default ON" "ON")
option(MYMPD_ENABLE_SSL "Enables OpenSSL usage, default ON" "ON")
option(MYMPD_MANPAGES "Creates and installs manpages" "ON")
option(MYMPD_STRIP_BINARY "Enables stripping the binaries for release build, default ON" "ON")
option(MYMPD_MINIMAL "Enables minimal myMPD build" "OFF")

if(MYMPD_MINIMAL)
  set(MYMPD_ENABLE_FLAC "OFF")
  set(MYMPD_ENABLE_IPV6 "OFF")
  set(MYMPD_ENABLE_LUA "OFF")
  set(MYMPD_ENABLE_LIBID3TAG "OFF")
  set(MYMPD_ENABLE_SSL "OFF")
endif()

# cmake modules
include(GNUInstallDirs)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)

# custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

# calculate paths
if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
  set(SUBDIR "/${PROJECT_NAME}")
  set(SUBDIRLIB "/lib")
  set(SUBDIRCACHE "/cache")
else()
  # for install in /opt
  set(SUBDIR "")
  set(SUBDIRLIB "")
  set(SUBDIRCACHE "")
endif()

message("Executables in: ${CMAKE_INSTALL_FULL_BINDIR}")

if(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
  set(MYMPD_WORK_DIR "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
else()
  set(MYMPD_WORK_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
endif()
message("Workdir: ${MYMPD_WORK_DIR}")

if(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
  set(MYMPD_CACHE_DIR "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
else()
  set(MYMPD_CACHE_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
endif()
message("Cachedir: ${MYMPD_CACHE_DIR}")

# set debug define for source
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG "ON")
else()
  set(DEBUG "OFF")
  set(NDEBUG "true")
endif()

if(MYMPD_EMBEDDED_ASSETS)
  message("Embedding assets in binary")
  set(MYMPD_DOC_ROOT "${MYMPD_WORK_DIR}/empty")
  set(MYMPD_LUALIBS_PATH "")
  set(ENV{MYMPD_BUILDDIR} "${CMAKE_CURRENT_BINARY_DIR}")
  execute_process(COMMAND "${PROJECT_SOURCE_DIR}/build.sh" createassets)
  # remove object files with embedded assets
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mympd.dir/src/web_server/utility.c.o")
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/mympd.dir/src/mympd_api/scripts.c.o")
else()
  message("Serving assets from filesystem")
  set(MYMPD_DOC_ROOT "${PROJECT_SOURCE_DIR}/htdocs")
  set(MYMPD_LUALIBS_PATH "${PROJECT_SOURCE_DIR}/contrib/lualibs")
  set(ENV{MYMPD_BUILDDIR} "${CMAKE_CURRENT_BINARY_DIR}")
  execute_process(COMMAND "${PROJECT_SOURCE_DIR}/build.sh" copyassets)
endif()
message("Document root: ${MYMPD_DOC_ROOT}")

# required dependencies
find_package(Threads REQUIRED)
find_package(PCRE2 REQUIRED)
find_library(MATH_LIB m REQUIRED)

# optional dependencies
if(MYMPD_ENABLE_SSL)
  message("Searching for openssl")
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
    if(NOT OPENSSL_VERSION VERSION_GREATER_EQUAL "1.1.0")
      message("OpenSSL is disabled because a version lower than 1.1.0 was found")
      set(MYMPD_ENABLE_SSL "OFF")
    endif()
  else()
    message("OpenSSL is disabled because it was not found")
    set(MYMPD_ENABLE_SSL "OFF")
  endif()
else()
  message("OpenSSL is disabled by user")
endif()

if(MYMPD_ENABLE_LIBID3TAG)
  message("Searching for libid3tag")
  find_package(LIBID3TAG)
  if(NOT LIBID3TAG_FOUND)
    message("Libid3tag is disabled because it was not found")
    set(MYMPD_ENABLE_LIBID3TAG "OFF")
  endif()
else()
  message("Libid3tag is disabled by user")
endif()

if(MYMPD_ENABLE_FLAC)
  message("Searching for flac")
  find_package(FLAC)
  if(NOT FLAC_FOUND)
    message("Flac is disabled because it was not found")
    set(MYMPD_ENABLE_FLAC "OFF")
  endif()
else()
  message("Flac is disabled by user")
endif()

if(MYMPD_ENABLE_LUA)
  if(EXISTS "/etc/alpine-release")
    set(ENV{LUA_DIR} "/usr/lib/lua5.4")
  endif()
  message("Searching for lua")
  find_package(Lua)
  if(LUA_FOUND)
    if(NOT LUA_VERSION_STRING VERSION_GREATER_EQUAL "5.3.0")
      message("Lua is disabled because a version lower than 5.3.0 was found")
      set(MYMPD_ENABLE_LUA "OFF")
    endif()
  else()
    message("Lua is disabled because it was not found")
    set(MYMPD_ENABLE_LUA "OFF")
  endif()
else()
  message("Lua is disabled by user")
endif()

# translation files
if(MYMPD_EMBEDDED_ASSETS)
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

# configure some files - version and path information
configure_file(src/compile_time.h.in "${PROJECT_BINARY_DIR}/compile_time.h")
configure_file(cmake/CopyConfig.cmake.in cmake/CopyConfig.cmake @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)

if(CMAKE_BUILD_TYPE MATCHES "(Release|Debug)")
  # set strict global compile flags
  add_compile_options(
    "-fstack-protector-strong"
    "-ffunction-sections"
    "-fdata-sections"
    "-Wall"
    "-Werror"
    "-Wvla"
    "-Winit-self"
    "-Wuninitialized"
    "-Wextra"
    "-pedantic"
    "-Wunused-parameter"
    "-Wshadow"
    "-Wwrite-strings"
    "-Wformat"
    "-Wformat-security"
    "-Wstrict-prototypes"
    "-Wold-style-definition"
    "-Wredundant-decls"
    "-Wnested-externs"
    "-Wmissing-include-dirs"
    "-Wundef"
    "-Wsign-compare"
  )

  # check for supported compiler flags
  foreach(FLAG IN ITEMS "-std=gnu17" "-fstack-clash-protection" "-fcf-protection" "-fno-plt")
    message("Checking for compiler flag ${FLAG}")
    unset(COMPILER_SUPPORTS_FLAG CACHE)
    unset(COMPILER_SUPPORTS_FLAG)
    check_c_compiler_flag("${FLAG}" COMPILER_SUPPORTS_FLAG)
    if(COMPILER_SUPPORTS_FLAG)
      add_compile_options("${FLAG}")
    endif()
  endforeach()

  if(NOT MYMPD_ENABLE_LIBASAN)
    #incompatible with libasan
    add_compile_options("-D_FORTIFY_SOURCE=2")
  endif()
else()
  # if CMAKE_BUILD_TYPE is neither Release nor Debug,
  # do not alter compile options
endif()

# libasan memory checker
# https://github.com/google/sanitizers/wiki/AddressSanitizer#faq
if(MYMPD_ENABLE_LIBASAN)
  message("Compiling with libasan")
  set(LIBASAN_FLAGS
    "-fsanitize=address"
    "-fsanitize=undefined"
    "-fsanitize=shift"
    "-fsanitize=integer-divide-by-zero"
    "-fsanitize=unreachable"
    "-fsanitize=vla-bound"
    "-fsanitize=null"
    "-fsanitize=return"
    "-fsanitize=signed-integer-overflow"
    "-fsanitize=bounds"
    "-fsanitize=bounds-strict"
    "-fsanitize=alignment"
    "-fsanitize=object-size"
    "-fsanitize=float-divide-by-zero"
    "-fsanitize=float-cast-overflow"
    "-fsanitize=nonnull-attribute"
    "-fsanitize=returns-nonnull-attribute"
    "-fsanitize=bool"
    "-fsanitize=enum"
    "-fsanitize=vptr"
    "-fno-omit-frame-pointer"
  )
  add_compile_options(
    ${LIBASAN_FLAGS}
    "-fno-omit-frame-pointer"
  )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(
    "-O2"
    "-fPIE"
  )
  if(NOT MYMPD_STRIP_BINARY)
    message("Generating binary with debug symbols")
    add_compile_options("-g")
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(
    "-ggdb"
    "-Og"
  )
else()
  # if CMAKE_BUILD_TYPE is neither Release nor Debug,
  # do not alter compile options
endif()

# linker flags
if(MYMPD_ENABLE_LIBASAN)
  add_link_options(${LIBASAN_FLAGS})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_link_options(
    "-Wl,-z,relro,-z,now,--gc-sections"
    "-pie"
  )
  if(MYMPD_STRIP_BINARY)
    message("Generating stripped binary")
    add_link_options("-s")
  endif()
else()
  # if CMAKE_BUILD_TYPE is neither Release nor Debug,
  # do not alter link options
endif()

# distributed libraries
add_subdirectory("dist")

# the main mympd target
add_subdirectory("src")

# command line utilities
add_subdirectory("cli_tools")

# link all together
target_link_libraries(mympd
  libmpdclient
  mjson
  mongoose
  rax
  sds
  tinymt
  ${CMAKE_THREAD_LIBS_INIT}
  ${MATH_LIB}
  ${PCRE2_LIBRARIES}
)

# link optional dependencies
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

# install
install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)
