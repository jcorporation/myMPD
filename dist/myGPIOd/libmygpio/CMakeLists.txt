# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

add_library(mygpio SHARED)

target_include_directories(mygpio
  PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
)

target_sources(mygpio
  PRIVATE
    src/buffer.c
    src/connection.c
    src/gpio_struct.c
    src/gpio.c
    src/gpioinfo.c
    src/gpiolist.c
    src/idle.c
    src/parser.c
    src/pair.c
    src/protocol.c
    src/socket.c
    src/util.c
)

set_target_properties(mygpio
  PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(mygpio
  mygpio-common
)

if (MYGPIOD_LIBRARY)
  # Install shared library
  install(TARGETS mygpio
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  # example
  add_subdirectory("example")
endif()

if (MYGPIOD_HEADER)
  # Install public headers
  install(DIRECTORY include/libmygpio
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # pkg-config file
  configure_file(libmygpio.pc.in libmygpio.pc @ONLY)
  install(FILES ${CMAKE_BINARY_DIR}/libmygpio/libmygpio.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
  )
endif()
