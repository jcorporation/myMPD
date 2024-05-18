---
layout: page
permalink: /scripting/functions/system_command
title: Execute a system command
---

Executes a system command and captures its output.

```lua
output = mympd.os_capture(command)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| command | string | system command to execute |
{: .table .table-sm }

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| output | string | system command output as lua string |
{: .table .table-sm }

If you want to run commands that changes the effective userid (e.g. with `sudo`) and you run myMPD with the default systemd service unit, you must create the mympd user manually and add an override.

```sh
groupadd -r mympd
useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd

curl -s https://raw.githubusercontent.com/jcorporation/myMPD/v10.0.0/contrib/initscripts/mympd.service.in | sed 's|@CMAKE_INSTALL_FULL_BINDIR@|/usr/bin|' /etc/systemd/system/mympd.service

systemctl daemon-reload
systemctl restart mympd
```
