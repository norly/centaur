centaur
=======

centaur is an ELF executable editing toolkit, focusing on code
injection and function detouring.


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

More examples can be found in the testsuite.


Build instructions, Credits, License, ...
-----------------------------------------

See the docs/ directory for all other documentation.
