Build instructions
==================

To build the CLI front-end, the static library and the shared library,
change to the top-level directory and run

    make

This will create a build/ directory containing the intermediary and
output files.


Dependencies and pkg-config
---------------------------

There is currently one hard dependency, libelf.

The Makefile uses pkg-config to try and autodetect the necessary
compiler and linker flags. Failing this, it defaults to "-lelf" to
link against libelf, and no additional include directories.

On Ubuntu, development files can be installed using
    apt-get install libelfg0-dev
for tired's libelf (preferred), or
    apt-get install libelf-dev
for Red Hat's libelf.


Documentation
-------------

Additional documentation can be generated from the source files using
Doxygen. If it is available on your machine, issue

    make docs

to build it in docs/.


Cleanup
-------

The usual
    make clean
and
    make distclean
are supported to clean binary files (clean), or all backup and
generated files (distclean).


Installation
------------

There is currently no automated installation.

If you need system-wide availability, you can copy
    include/libelfu --> /usr/local/include/
    build/elfucli --> /usr/local/bin/
    build/libelfu.{a,so*} --> /usr/local/lib/
or your local variation thereof.
