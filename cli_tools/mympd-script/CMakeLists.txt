# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_executable(mympd-script "")

target_include_directories(mympd-script
  PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
)

target_sources(mympd-script
  PRIVATE
    mympd-script.c
    ../log.c
    ../../src/lib/cacertstore.c
    ../../src/lib/convert.c
    ../../src/lib/filehandler.c
    ../../src/lib/http_client.c
    ../../src/lib/list.c
    ../../src/lib/sds_extras.c
    ../../src/lib/passwd.c
    ../../src/lib/random.c
)

target_link_libraries(mympd-script
  sds
  mongoose
  ${OPENSSL_LIBRARIES}
)

install(TARGETS mympd-script DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
