#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>

#include "elfhandle.h"
#include "options.h"
#include "printing.h"


int main(int argc, char **argv)
{
  CLIOpts opts = { 0 };
  ELFHandles hIn = { 0 };
  ELFHandles hOut = { 0 };
  int exitval = EXIT_SUCCESS;

  /* Is libelf alive and well? */
  if (elf_version(EV_CURRENT) == EV_NONE) {
    fprintf(stderr, "libelf init error: %s\n", elf_errmsg(-1));
  }


  /* Parse and validate user input */
  parseOptions(&opts, argc, argv);


  /* Open input/output files */
  openElf(&hIn, opts.fnInput, ELF_C_READ);
  if (!hIn.e) {
    exitval = EXIT_FAILURE;
    goto EXIT;
  }

  if (opts.fnOutput) {
    openElf(&hOut, opts.fnOutput, ELF_C_WRITE);
    if (!hOut.e) {
      exitval = EXIT_FAILURE;
      goto EXIT;
    }
  }


  /* Now that we have a (hopefully) sane environment, execute commands */
  if (opts.printHeader) {
    printHeader(hIn.e);
  }

  if (opts.printSegments) {
    printSegments(hIn.e);
  }

  if (opts.printSections) {
    printSections(hIn.e);
  }


  /* Copy the input ELF to the output file if the latter is specified */
  if (opts.fnOutput) {
    ElfuElf *me;

    me = elfu_modelFromElf(hIn.e);

    if (me) {
      printf("Model successfully loaded.\n");

      elfu_modelToElf(me, hOut.e);

      printf("Model converted to ELF, ready to be written.\n");
    } else {
      printf("Failed to load model.\n");
    }
  }



EXIT:
  if (hOut.e) {
    if (elf_update(hOut.e, ELF_C_WRITE) < 0) {
      fprintf(stderr, "elf_update() failed: %s\n", elf_errmsg(-1));
    }
    closeElf(&hOut);
  }

  if (hIn.e) {
    closeElf(&hIn);
  }

  return (exitval);
}
