Command line options
====================

You can set some basic options with command line options. All these
options have sane default values and should not be changed for default
usage.

The ``workdir`` and ``cachedir`` options are useful if you want to run
more then one instance of myMPD on the same host.

+------------------------+---------------------------------------------+
| OPTION                 | DESCRIPTION                                 |
+========================+=============================================+
| ``-c``, ``--config``   | creates config and ssl certificates and     |
|                        | exits (default directory:                   |
|                        | ``/var/lib/mympd/config/``)                 |
+------------------------+---------------------------------------------+
| ``-h``, ``--help``     | displays this help                          |
+------------------------+---------------------------------------------+
| ``-v``, ``--version``  | displays this help                          |
+------------------------+---------------------------------------------+
| ``-s``, ``--syslog``   | enable syslog logging (facility: daemon)    |
+------------------------+---------------------------------------------+
| ``-w``,                | working directory (default:                 |
| ``--workdir <path>``   | ``/var/lib/mympd``). This folder must       |
|                        | exist, if not started as root.              |
+------------------------+---------------------------------------------+
| ``-a``,                | cache directory (default:                   |
| ``--cachedir <path>``  | ``/var/cache/mympd``). This folder must     |
|                        | exist, if not started as root.              |
+------------------------+---------------------------------------------+
| ``-p``, ``--pin``      | sets a pin for myMPD settings               |
+------------------------+---------------------------------------------+
