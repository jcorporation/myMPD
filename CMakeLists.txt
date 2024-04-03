# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

# minimal cmake version needed for new option handling
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0003 NEW)

# myMPD is written in C
# supported compilers: gcc, clang
project(mympd
  VERSION 14.1.1
  LANGUAGES C
)

# output binaries in bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message("Cmake version: ${CMAKE_VERSION}")
message("Cmake src dir: ${PROJECT_SOURCE_DIR}")
message("Cmake build dir: ${CMAKE_CURRENT_BINARY_DIR}")
message("Cmake build type: ${CMAKE_BUILD_TYPE}")
message("Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")

# reset cmake default flags
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "")
set(CMAKE_C_FLAGS_MINSIZEREL "")

# default is to deliver the assets from htdocs for Debug
# and embedded assets for all other build types
if(NOT DEFINED MYMPD_EMBEDDED_ASSETS)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MYMPD_EMBEDDED_ASSETS "OFF")
  else()
    set(MYMPD_EMBEDDED_ASSETS "ON")
  endif()
endif()

# available options
option(MYMPD_BUILD_TESTING "Enables building of unit tests" "OFF")
option(MYMPD_DOC "Installs documentation, default ON" "ON")
option(MYMPD_DOC_HTML "Creates and installs the html documentation, default OFF" "OFF")
option(MYMPD_EMBEDDED_ASSETS "Embed assets in binary, default ON, OFF for Debug" "ON")
option(MYMPD_ENABLE_EXPERIMENTAL "Enable experimental features" "OFF")
option(MYMPD_ENABLE_FLAC "Enables flac support, default ON" "ON")
option(MYMPD_ENABLE_IPV6 "Enables IPv6, default ON" "ON")
option(MYMPD_ENABLE_LIBID3TAG "Enables libid3tag support, default ON" "ON")
option(MYMPD_ENABLE_LUA "Enables lua support, default ON" "ON")
option(MYMPD_MANPAGES "Creates and installs manpages" "ON")
option(MYMPD_MINIMAL "Enables minimal myMPD build, disables all MYMPD_ENABLE_* flags" "OFF")
option(MYMPD_STARTUP_SCRIPT "Installs the startup script, default ON" "ON")
# sanitizer options
option(MYMPD_ENABLE_ASAN "Enables build with address sanitizer, default OFF" "OFF")
option(MYMPD_ENABLE_TSAN "Enables build with thread sanitizer, default OFF" "OFF")
option(MYMPD_ENABLE_UBSAN "Enables build with undefined behavior sanitizer, default OFF" "OFF")

if(MYMPD_MINIMAL)
  set(MYMPD_ENABLE_FLAC "OFF")
  set(MYMPD_ENABLE_IPV6 "OFF")
  set(MYMPD_ENABLE_LUA "OFF")
  set(MYMPD_ENABLE_LIBID3TAG "OFF")
endif()

if(MYMPD_ENABLE_EXPERIMENTAL)
  message("Experimental features enabled")
endif()

# cmake modules
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckIPOSupported)
include(GNUInstallDirs)

# custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

# calculate paths
if(CMAKE_INSTALL_PREFIX MATCHES "/usr")
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
  set(MYMPD_CACHE_DIR "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
else()
  set(MYMPD_WORK_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
  set(MYMPD_CACHE_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
endif()

message("Workdir: ${MYMPD_WORK_DIR}")
message("Cachedir: ${MYMPD_CACHE_DIR}")

# create assets and set doc root
set(ENV{MYMPD_BUILDDIR} "${CMAKE_CURRENT_BINARY_DIR}")

if(MYMPD_EMBEDDED_ASSETS)
  message("Embedding assets in binary")
  set(MYMPD_DOC_ROOT "${MYMPD_WORK_DIR}/empty")
  set(MYMPD_LUALIBS_PATH "")
  execute_process(COMMAND "${PROJECT_SOURCE_DIR}/build.sh" createassets RESULT_VARIABLE RC_CREATE_ASSETS)
  # remove object files with embedded assets
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/src/CMakeFiles/mympd.dir/web_server/utility.c.o")
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/src/CMakeFiles/mympd.dir/mympd_api/scripts.c.o")
else()
  message("Serving assets from filesystem")
  set(MYMPD_DOC_ROOT "${PROJECT_SOURCE_DIR}/htdocs")
  set(MYMPD_LUALIBS_PATH "${PROJECT_SOURCE_DIR}/contrib/lualibs")
  execute_process(COMMAND "${PROJECT_SOURCE_DIR}/build.sh" copyassets RESULT_VARIABLE RC_CREATE_ASSETS)
endif()

if(RC_CREATE_ASSETS GREATER 0)
  message(FATAL_ERROR "Creating assets failed")
endif()

message("Document root: ${MYMPD_DOC_ROOT}")
message("Docdir: ${CMAKE_INSTALL_FULL_DOCDIR}")

# required dependencies
find_package(Threads REQUIRED)
find_package(PCRE2 REQUIRED)
find_library(MATH_LIB m REQUIRED)
find_package(OpenSSL REQUIRED)

# check dependencies versions
if(OPENSSL_VERSION VERSION_LESS "1.1.0")
  message(FATAL_ERROR "myMPD requires an OpenSSL version greater or equal 1.1.0")
endif()

# optional dependencies
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
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/bg-BG.json.gz")
    set(I18N_bg_BG "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/de-DE.json.gz")
    set(I18N_de_DE "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/en-US.json.gz")
    set(I18N_en_US "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/es-AR.json.gz")
    set(I18N_es_AR "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/es-ES.json.gz")
    set(I18N_es_ES "ON")
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
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/pl-PL.json.gz")
    set(I18N_pl_PL "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/ru-RU.json.gz")
    set(I18N_ru_RU "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/zh-Hans.json.gz")
    set(I18N_zh_Hans "ON")
  endif()
  if(EXISTS "${PROJECT_BINARY_DIR}/htdocs/assets/i18n/zh-Hant.json.gz")
    set(I18N_zh_Hant "ON")
  endif()
endif()

# create html documentation
if(MYMPD_DOC_HTML)
  execute_process(COMMAND "${PROJECT_SOURCE_DIR}/build.sh" doc "${PROJECT_BINARY_DIR}/htmldoc" RESULT_VARIABLE RC_CREATE_DOC)
endif()
if(RC_CREATE_DOC GREATER 0)
  message(FATAL_ERROR "Creating documentation failed")
endif()

# configure some files - version and path information
configure_file(src/compile_time.h.in "${PROJECT_BINARY_DIR}/compile_time.h")
configure_file(cmake/Install.cmake.in cmake/Install.cmake @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)
configure_file(contrib/initscripts/mympd.freebsdrc.in contrib/initscripts/mympd.freebsdrc @ONLY)

# global compile options
add_compile_options(
  "-DMPACK_HAS_CONFIG=1"
)
# set myMPD specific debug define
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(
    "-DMYMPD_DEBUG=ON"
  )
endif()

if(CMAKE_BUILD_TYPE MATCHES "(Debug|Release|RelWithDebInfo|MinSizeRel)")
  # set strict global compile flags
  add_compile_options(
    "-fdata-sections"
    "-ffunction-sections"
    "-fstack-protector-strong"
    "-pedantic"
    "-Wall"
    "-Werror"
    "-Wextra"
    "-Wformat"
    "-Wformat-security"
    "-Winit-self"
    "-Wmissing-include-dirs"
    "-Wnested-externs"
    "-Wold-style-definition"
    "-Wredundant-decls"
    "-Wshadow"
    "-Wsign-compare"
    "-Wstrict-prototypes"
    "-Wundef"
    "-Wuninitialized"
    "-Wunused-parameter"
    "-Wvla"
    "-Wwrite-strings"
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

  if(NOT MYMPD_ENABLE_ASAN AND NOT MYMPD_ENABLE_UBSAN)
    # incompatible with address sanitizers
    add_compile_options("-D_FORTIFY_SOURCE=2")
  endif()
else()
  # if CMAKE_BUILD_TYPE is neither Release nor Debug,
  # do not alter compile options
endif()

# sanitizers
set(ASAN_FLAGS
  "-fsanitize=address"
  "-fsanitize=leak"
)

set(UBSAN_FLAGS
  "-fsanitize=undefined"
)

set(TSAN_FLAGS
  "-fsanitize=thread"
)

if(MYMPD_ENABLE_ASAN)
  message("Compiling with address sanitizer")
  add_compile_options(
    ${ASAN_FLAGS}
    "-fno-omit-frame-pointer"
  )
elseif(MYMPD_ENABLE_UBSAN)
  message("Compiling with undefined behavior sanitizer")
  add_compile_options(
    ${UBSAN_FLAGS}
    "-fno-omit-frame-pointer"
  )
elseif(MYMPD_ENABLE_TSAN)
  message("Compiling with thread sanitizer")
  add_compile_options(
    ${TSAN_FLAGS}
    "-fno-omit-frame-pointer"
  )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(
    "-ggdb"
    "-Og"
  )
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
elseif(CMAKE_BUILD_TYPE MATCHES "(Release|RelWithDebInfo|MinSizeRel)")
  add_compile_options(
    "-fPIE"
    "-O2"
  )
  if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("Generating binary with debug symbols")
    add_compile_options("-g")
  endif()
  # IPO/LTO support
  check_ipo_supported()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  # if CMAKE_BUILD_TYPE is not Debug, Release, RelWithDebInfo or MinSizeRel
  # do not alter compile options
endif()

# linker flags
if(MYMPD_ENABLE_ASAN)
  add_link_options(${ASAN_FLAGS})
elseif(MYMPD_ENABLE_UBSAN)
  add_link_options(${UBSAN_FLAGS})
elseif(MYMPD_ENABLE_TSAN)
  add_link_options(${TSAN_FLAGS})
endif()

if(CMAKE_BUILD_TYPE MATCHES "(Release|RelWithDebInfo|MinSizeRel)")
  add_link_options(
    "-pie"
    "-Wl,-z,relro,-z,now,--gc-sections,--as-needed"
  )
  if(CMAKE_BUILD_TYPE MATCHES "(Release|MinSizeRel)")
    message("Generating stripped binary")
    add_link_options("-s")
  endif()
else()
  # if CMAKE_BUILD_TYPE is not Release, RelWithDebInfo or MinSizeRel
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
  libmympdclient
  mjson
  mpack
  mongoose
  rax
  sds
  ${CMAKE_THREAD_LIBS_INIT}
  ${MATH_LIB}
  ${PCRE2_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

# link optional dependencies
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
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/Install.cmake)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/cli_tools/mympd-config/mympd-config DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})

# test
if(MYMPD_BUILD_TESTING)
  include(CTest)
  add_subdirectory(test)
endif()
