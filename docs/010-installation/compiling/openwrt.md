---

title: OpenWrt
---

## Building with SDK

1. Download desired version of OpenWrt SDK for Your device from: [https://openwrt.org/downloads](https://openwrt.org/downloads)
   The SDK must match the version of OpenWrt installed on Your device.
2. Unpack SDK and change current directory to it.
3. Run following commands to download dependencies recipes:
    ```
    scripts/feeds update -a
    scripts/feeds install libflac libid3tag liblua5.3 libopenssl libpcre2
    ```
4. Copy contents of `contrib/packaging/openwrt` from myMPD tree
   to `package/mympd` directory of SDK.
5. To build package run:
    ```
    make -j$(nproc) BUILD_LOG=1
    ```
6. Resulting package will be placed in `bin` directory.

## Building in full OpenWrt buildroot

1. Clone the OpenWrt tree [https://git.openwrt.org/openwrt/openwrt.git](https://git.openwrt.org/openwrt/openwrt.git)
2. Run following commands to download dependencies recipes:
    ```
    scripts/feeds update -a
    scripts/feeds install libflac libid3tag liblua5.3 libopenssl libpcre2
    ```
3. Copy contents of `contrib/packaging/openwrt` from myMPD tree to `package/mympd` directory of SDK.
4. To select myMPD package build run:
    - `make menuconfig`
    - Select it in 'Sound' menu, to build it run:
    - make -j$(nproc) BUILD_LOG=1`
6. Resulting package will be placed in `bin` directory.

***

Thanks goes to [tmn505](https://github.com/tmn505) for the OpenWrt support.
