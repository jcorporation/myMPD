---
title: Debian and Ubuntu Installation
---

When installing on Debian or Ubuntu, the easiest way is to configure your package manager to securely download from the official myMPD repository. It allows you to use apt's native update mechanism to upgrade myMPD instead of having to manually download the deb package for each new release.

!!! note
    Simply replace `Debian` with `xUbuntu` for Ubuntu installation.

```sh
# Get Debian VERSION_ID from os-release file
source /etc/os-release
echo $VERSION_ID

# Download the signing key and install it in a dedicated keyring
curl http://download.opensuse.org/repositories/home:/jcorporation/Debian_$VERSION_ID/Release.key | gpg --no-default-keyring --keyring /usr/share/keyrings/jcorporation.github.io.gpg --import

# ⚠️ VERIFY the fingerprint of the downloaded key (A37A ADC4 0A1C C6BE FB75  372F AA09 B8CC E895 BD7D - home:jcorporation OBS Project <home:jcorporation@build.opensuse.org>) 
gpg --no-default-keyring --keyring /usr/share/keyrings/jcorporation.github.io.gpg --fingerprint

# Make the imported keyring world-readable
chmod 644 /usr/share/keyrings/jcorporation.github.io.gpg

# Add the myMPD APT repository and ensure releases are signed with the repository's official keys
cat <<EOF > /etc/apt/sources.list.d/jcorporation.list
deb [signed-by=/usr/share/keyrings/jcorporation.github.io.gpg] http://download.opensuse.org/repositories/home:/jcorporation/Debian_$VERSION_ID/ ./
EOF
cat /etc/apt/sources.list.d/jcorporation.list

# Install MyMPD
apt update
apt install mympd
```
