---
layout: page
permalink: /running
title: Running
---

## Startup script
The installation process installs a LSB compatible startup script / systemd unit for myMPD.

| INIT SYSTEM | SCRIPT |
| ----------- | ------ |
| sysVinit | `/etc/init.d/mympd` |
| open-rc | `/etc/init.d/mympd` |
| systemd | `/usr/lib/systemd/system/mympd` or `/lib/systemd/system/mympd` |

### Systemd usage

You must enable and start the service manually. Use `systemctl enable mympd` to enable myMPD at startup and `systemctl start mympd` to start myMPD now.

myMPD logs to STDERR, you can see the logs with `journalctl -u mympd`.

## Manual startup

To start myMPD in the actual console session: `mympd` (myMPD logs to the console)

Description of [Commandline-Options]({{ site.baseurl }}/configuration/).

## Docker

Goto [Docker]({{ site.baseurl }}/installation/docker)
