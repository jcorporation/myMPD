#
# Copyright (C) 2021 Tomasz Maciej Nowak <tmn505@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

include $(TOPDIR)/rules.mk

PKG_NAME := mympd
PKG_VERSION := 10.4.0
PKG_RELEASE := 1

PKG_SOURCE := $(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL := https://codeload.github.com/jcorporation/myMPD/tar.gz/refs/tags/v$(PKG_VERSION)?
PKG_HASH := skip

PKG_LICENSE := GPL-3.0
PKG_LICENSE_FILE := LICENSE.md

PKG_MAINTAINER := Juergen Mang <mail [at] jcgames [dot] de>

PKG_BUILD_DIR := $(BUILD_DIR)/myMPD-$(PKG_VERSION)
PKG_BUILD_PARALLEL := 1
PKG_FORTIFY_SOURCE := 0
PKG_CONFIG_DEPENDS := \
    CONFIG_MYMPD_FLAC \
    CONFIG_MYMPD_ID3TAG \
    CONFIG_MYMPD_LUA

CMAKE_BINARY_SUBDIR := release

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/mympd
  SECTION := sound
  CATEGORY := Sound
  TITLE := myMPD - web MPD client and more
  URL := https://jcorporation.github.io
  DEPENDS := +libpcre2 +MYMPD_FLAC:libflac +MYMPD_ID3TAG:libid3tag +MYMPD_LUA:liblua5.3 +libopenssl
  USERID := mympd:mympd
endef

define Package/mympd/description
Standalone and mobile friendly web MPD client with a tiny footprint and advanced features.
endef

define Package/mympd/config
    if PACKAGE_mympd

        config MYMPD_FLAC
            bool "Extracting cover images from FLAC files"
            default y

        config MYMPD_ID3TAG
            bool "Extracting cover images from ID3 tags"
            default y

        config MYMPD_LUA
            bool "LUA scripts support"
            default n

    endif
endef

CMAKE_OPTIONS += \
    -DCMAKE_BUILD_TYPE=Release \
    -DMYMPD_ENABLE_FLAC=$(if $(CONFIG_MYMPD_FLAC),ON,OFF) \
    -DMYMPD_ENABLE_LIBID3TAG=$(if $(CONFIG_MYMPD_ID3TAG),ON,OFF) \
    -DMYMPD_ENABLE_LUA=$(if $(CONFIG_MYMPD_LUA),ON,OFF)

define Package/mympd/install
    $(INSTALL_DIR) $(1)/usr/bin
    $(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/mympd $(1)/usr/bin/
    $(if $(CONFIG_MYMPD_LUA),$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/mympd-script $(1)/usr/bin/)

    $(INSTALL_DIR) $(1)/etc/config
    $(INSTALL_CONF) ./files/mympd.config $(1)/etc/config/mympd
    $(INSTALL_DIR) $(1)/etc/init.d
    $(INSTALL_BIN) ./files/mympd.init $(1)/etc/init.d/mympd
endef

$(eval $(call BuildPackage,mympd))
