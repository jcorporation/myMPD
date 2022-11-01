---
layout: page
permalink: /installation/compiling/termux
title: Termux
---

1. Update the packages
```
pkg upgrade
```
2. Install dependencies
```
pkg install build-essential cmake perl pcre2 openssl libid3tag libflac lua54 git
```

## Now to compile it

3. Clone the git repo (depth makes it download only the last commit making it much smaller and faster to download)
```
git clone https://github.com/jcorporation/myMPD.git --depth=1
```
4. Change directory to the git repo and run build
```
cd myMPD
cmake -B release -DCMAKE_BUILD_TYPE=Release -DLUA_MATH_LIBRARY=/system/lib64/libm.so .
# substitute 'lib64' with 'lib' if you are on 32bit arch
make -C release
```

## Initial config

5. Create the config directory for myMPD and run it to create the config files from the environment variables
```
mkdir -p $HOME/.config/mympd

export MYMPD_HTTP_PORT=8080 # you can use '80' if using root
export MYMPD_LOGLEVEL=6
export MPD_HOST=/data/data/com.termux/files/usr/var/run/mpd.socket # this is default for termux i think but check your /etc/mpd.conf value for bind_to_address
export MPD_PORT=35000 # choose whatever mpd you use in /etc/mpd.conf
export MYMPD_SSL=false # 'true' if using root
export MYMPD_SSL_PORT=443

$HOME/myMPD/release/mympd -w $HOME/.config/mympd # run it
```

## Running it

After this run myMPD with just this, the rest is necessary for the first start only
```
$HOME/myMPD/release/mympd -w $HOME/.config/mympd
```

## Running with root (if you want to use lower ports, or use SSL)

I made this little script which should run mympd as root with little trouble **(run it as the user!)**
```
#!/bin/bash
su root -c "$HOME/myMPD/release/mympd -w $HOME/.config/mympd -u $(whoami)"
```
You can also prepend `nohup` before `su` to make it start in background and not stop even after closing the terminal

***
Thanks goes to [sandorex](https://github.com/sandorex) for the Termux support.

[GitHub Disussion](https://github.com/jcorporation/myMPD/discussions/612)
