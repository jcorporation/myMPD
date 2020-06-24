#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 3.4)

include(GNUInstallDirs)

project (cli_tools C)
set(CPACK_PACKAGE_VERSION_MAJOR "6")
set(CPACK_PACKAGE_VERSION_MINOR "5")
set(CPACK_PACKAGE_VERSION_PATCH "0")

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
include(CheckCSourceCompiles)

if(${MEMCHECK} MATCHES "TRUE")
  #for use with valgrind
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -Werror -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
        -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
        -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
        -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
        -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
endif()

set(MYMPD_COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
        -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#linker flags
set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

#
#mympd-config utility
#

set_property(SOURCE "mympd-config.c" PROPERTY COMPILE_FLAGS ${MYMPD_COMPILE_FLAGS})

set(MYMPD_CONFIG_SOURCES
  mympd-config.c
  ../dist/src/sds/sds.c
  ../src/sds_extras.c
)

add_executable(mympd-config ${MYMPD_CONFIG_SOURCES})
target_link_libraries(mympd-config)
install(TARGETS mympd-config DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})

#
#mympd-script utility
#

if (LUA_FOUND)
  set(MYMPD_SCRIPT_SOURCES
    mympd-script.c
    ../dist/src/mongoose/mongoose.c
    ../dist/src/sds/sds.c
    ../src/sds_extras.c
  )

  set(MONGOOSE_FLAGS "${MONGOOSE_SSL} -DMG_DISABLE_MQTT -DMG_DISABLE_MQTT_BROKER -DMG_DISABLE_DNS_SERVER \
	-DMG_DISABLE_COAP -DMG_DISABLE_HTTP_CGI -DMG_DISABLE_HTTP_SSI -DMG_DISABLE_SSI_EXEC \
	-DMG_DISABLE_BROADCAST -DMG_DISABLE_THREADS -DMG_DISABLE_HTTP_DIGEST_AUTH -D CS_DISABLE_MD5 \
	-DMG_DISABLE_HTTP_WEBSOCKET")

  set_property(SOURCE "mympd-script.c" PROPERTY COMPILE_FLAGS "${MYMPD_COMPILE_FLAGS} ${MONGOOSE_FLAGS}")
  set_property(SOURCE "../dist/src/mongoose/mongoose.c" PROPERTY COMPILE_FLAGS "-Wno-format-truncation ${MONGOOSE_FLAGS}")

  add_executable(mympd-script ${MYMPD_SCRIPT_SOURCES})
  target_link_libraries(mympd-script)
  if (OPENSSL_FOUND)
    target_link_libraries(mympd-script ${OPENSSL_LIBRARIES})
  endif()

  install(TARGETS mympd-script DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
endif()