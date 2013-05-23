#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>

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
  ElfuElf *me;

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


  /* Now that we have a (hopefully) sane environment, execute commands.
   * Printing will have to be reimplemented based on the memory model.
   */
  if (opts.printHeader) {
    printHeader(hIn.e);
  }

  if (opts.printSegments) {
    printSegments(hIn.e);
  }

  if (opts.printSections) {
    printSections(hIn.e);
  }


  me = elfu_mFromElf(hIn.e);
  if (me) {
    closeElf(&hIn);
    printf("Model successfully loaded.\n");
    elfu_mCheck(me);
    printf("Input model checked.\n");
  } else {
    printf("Failed to load model, aborting.\n");
    goto EXIT;
  }


  /* Copy the input ELF to the output file if the latter is specified.
   * Perform requested transformations on the memory model on-the-fly. */
  if (!opts.fnOutput) {
    printf("No output file specified - no further operations performed.\n");
  } else {
    if (opts.expandNobitsOffs) {
      elfu_mExpandNobits(me, opts.expandNobitsOffs);
    }

    if (opts.insertBeforeSz) {
      elfu_mInsertBefore(me, opts.insertBeforeOffs, opts.insertBeforeSz);
    }

    if (opts.insertAfterSz) {
      elfu_mInsertAfter(me, opts.insertAfterOffs, opts.insertAfterSz);
    }

    elfu_mCheck(me);
    printf("Output model checked.\n");


    openElf(&hOut, opts.fnOutput, ELF_C_WRITE);
    if (!hOut.e) {
      printf("Failed to open output file. Aborting.\n");
      exitval = EXIT_FAILURE;
      goto EXIT;
    }

    elfu_mToElf(me, hOut.e);
    printf("Model converted to ELF, ready to be written.\n");
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
