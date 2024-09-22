---

title: Logging
---

myMPD logs in the default configuration to the console. You can specify the `-s` command line option to log to syslog (facility: daemon).

The log levels are unix default.

| LEVEL | DESCRIPTION |
| ----- | ----------- |
| 0 | emerg |
| 1 | alert |
| 2 | critical |
| 3 | error |
| 4 | warn |
| 5 | notice (default) |
| 6 | info |
| 7 | debug |


If you want to start myMPD with a different loglevel as configured you can set the `MYMPD_LOGLEVEL` environment variable accordingly.
