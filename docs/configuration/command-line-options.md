---
layout: page
permalink: /configuration/command-line-options
title: Command line options
---

You can set some basic options with command line options. All these options have sane default values and should not be changed for default usage.

The `workdir` and `cachedir` options are useful if you want to run more then one instance of myMPD on the same host.

| OPTION | DESCRIPTION |
| ------ | ----------- |
| `-c`, `--config` | creates config and ssl certificates and exits (default directory: `/var/lib/mympd/config/`) |
| `-h`, `--help` | displays this help |
| `-v`, `--version` | displays this help |
| `-u`, `--user <username>`| username to drop privileges to (default: `mympd`) |
| `-s`, `--syslog` | enable syslog logging (facility: daemon) |
| `-w`, `--workdir <path>` | working directory (default: `/var/lib/mympd`) |
| `-a`, `--cachedir <path>` | cache directory (default: `/var/cache/mympd`) |
| `-p`, `--pin` | sets a pin for myMPD settings |
{: .table .table-sm }

- Setting a pin is only supported with compiled in ssl support
