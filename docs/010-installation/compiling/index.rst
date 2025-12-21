Compiling
=========

Releases
--------

+-------------------+----------+-------------------------------------------------------------------------------------+
| NAME              | STATE    | DESCRIPTION                                                                         |
+===================+==========+=====================================================================================+
| `latest release`_ | stable   | The latest stable release, this is the preferred image for daily, hassle-free usage |
+-------------------+----------+-------------------------------------------------------------------------------------+
| `master`_         | stable   | the latest releas is created from the master branch                                 |
+-------------------+----------+-------------------------------------------------------------------------------------+
| `devel`_          | unstable | this branch is for the next bugfix release                                          |
+-------------------+----------+-------------------------------------------------------------------------------------+
| other branches    | unstable | development branches for new major and minor releases                               |
+-------------------+----------+-------------------------------------------------------------------------------------+

.. _latest release: https://github.com/jcorporation/myMPD/releases/latest
.. _master: https://github.com/jcorporation/myMPD/tree/master
.. _devel: https://github.com/jcorporation/myMPD/tree/devel

Get the appropriated tarball or clone the git repository and checkout the wanted branch.

.. code-block:: sh
   :caption: `Example: Clone and use devel branch`

   git clone https://github.com/jcorporation/myMPD.git
   git checkout devel

Build Dependencies
------------------

myMPD has only a few dependencies beside the standard c libraries. Not installing the optional dependencies leads only to a smaller subset of myMPD functions.

- cmake >= 3.13
- libasan3 - for memcheck builds only
- Perl - to create translation files
- gzip - to precompress assets
- jq - json parsing
- lua - to precompile embedded lua libraries
- whiptail - for mympd-config
- Devel packages:
    - pcre2 - for pcre support
    - OpenSSL >= 1.1.0 - for https support
    - Optional:
        - libid3tag - to extract embedded coverimages and lyrics
        - flac - to extract embedded coverimages and lyrics
        - liblua >= 5.3.0 - for myMPD scripting
        - libmygpio - for GPIO scripting functions
        - libmpdclient - embedded libmpdclient is used if it was not found or is too old.
        - utf8proc - for utf8 support
- Documentation:
    - Doxygen
    - JSDoc
    - Ldoc
    - mkdocs

You can type ``./build.sh installdeps`` as root to install the dependencies (works only for supported distributions). For all other distributions you must install the packages manually.

libmpdclient
~~~~~~~~~~~~

myMPD requires a very recent `libmpdclient <https://github.com/MusicPlayerDaemon/libmpdclient>`__ version. This version is distributed in the myMPD source tree and used if no libmpdclient was found or the found library is too old.

Building myMPD
--------------

.. toctree::
   :titlesonly:

   build-sh.rst
   cmake.rst
   termux.rst
   openwrt.rst
   freebsd.rst
