centaur
=======

centaur is an ELF executable editing toolkit, focusing on code
injection and function detouring.

It has been tested successfully in the environments provided by
several Linux distributions, on both x86-32 and x86-64 machines:

  - Ubuntu 12.04 LTS        (x86-64)
  - Arch Linux (June 2013)  (x86-64)
  - Slackware 14.0          (x86-32)


Example
-------

Injecting an object file into a program and detouring a function
could hardly be simpler:

    elfucli --input program \
            --reladd objfile.o \
            --detour oldfunc,newfunc \
            --output program_modified

elfucli parses the command line parameters one by one like a script.

In this example, it:

  1. Loads the executable `program` containing the function `oldfunc`.
  2. Injects an object file containing the function `newfunc`.
  3. Overwrites the beginning of `oldfunc` with a jump to `newfunc`.
  4. Writes the modified program to `program_modified`.

This functionality is exposed by the underlying `libelfu` via a C API,
at the same high level. `elfucli` serves as an example application for
it and doubles as a handy scalpel for ELF files.

More examples can be found in the testsuite. The 'detour' test is
particularly similar to the example above.


Build instructions, testing
---------------------------

Usually, a plain

    make

should be enough to build centaur, provided that a version of libelf
and its development files are installed. If not, see docs/building.md
for further hints such as the packages to be installed on Ubuntu.

Once that is done,

    make check

will build and run the testsuite. See docs/tests.md for details.


License, etc
------------

See the docs/ directory for all other documentation.
