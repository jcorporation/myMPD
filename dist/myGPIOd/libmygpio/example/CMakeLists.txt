# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_executable(mygpio-example "")

target_include_directories(mygpio-example
  PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
)

target_sources(mygpio-example
  PRIVATE
    main.c
)

target_link_directories(mygpio-example
  PRIVATE
    "${PROJECT_BINARY_DIR}/libmygpio"
)

target_link_libraries(mygpio-example
  PRIVATE
    "mygpio"
)
