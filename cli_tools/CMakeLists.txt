#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

project (cli_tools C)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
configure_file(mympd_config_defs.h.in ${PROJECT_BINARY_DIR}/mympd_config_defs.h)

#mympd-script utility
if (LUA_FOUND)
  set(MYMPD_SCRIPT_SOURCES
    mympd-script.c
    log.c
    ../dist/mongoose/mongoose.c
    ../dist/sds/sds.c
    ../src/lib/sds_extras.c
    ../src/lib/http_client.c
  )

  #compiler flags for mongoose
  set_property(SOURCE ../dist/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral")
  #compiler flags for sds
  set_property(SOURCE ../dist/sds/sds.c PROPERTY COMPILE_FLAGS "-Wno-shadow -Wno-format-nonliteral")

  add_executable(mympd-script ${MYMPD_SCRIPT_SOURCES})
  target_link_libraries(mympd-script)
  if (OPENSSL_FOUND)
    target_link_libraries(mympd-script ${OPENSSL_LIBRARIES})
  endif()

  install(TARGETS mympd-script DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
endif()
