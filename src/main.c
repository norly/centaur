#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
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

  /* Now that we have a (hopefully) sane environment, execute commands. */
  me = elfu_mFromElf(hIn.e);
  closeElf(&hIn);
  if (!me) {
    printf("Failed to load model, aborting.\n");
    goto EXIT;
  }

  elfu_mCheck(me);

  //elfu_mDumpElf(me);

  /* Perform requested transformations on the memory model on-the-fly. */
  if (opts.fnReladd) {
    ELFHandles hRel = { 0 };
    ElfuElf *mrel = NULL;

    openElf(&hRel, opts.fnReladd, ELF_C_READ);
    if (!hRel.e) {
      printf("--reladd: Failed to open file for --reladd, skipping operation.\n");
    } else {
      mrel = elfu_mFromElf(hRel.e);
      closeElf(&hRel);
      if (!mrel) {
        printf("--reladd: Failed to load model for --reladd, skipping operation.\n");
      } else {
        elfu_mCheck(mrel);
        elfu_mReladd(me, mrel);
        printf("--reladd: Injected %s.\n", opts.fnReladd);
      }
    }
  }

  //elfu_mDumpElf(me);

  /* Copy the input ELF to the output file if one is specified. */
  if (opts.fnOutput) {
    printf("Writing modified file to %s.\n", opts.fnOutput);
    elfu_mCheck(me);


    openElf(&hOut, opts.fnOutput, ELF_C_WRITE);
    if (!hOut.e) {
      printf("Failed to open output file. Aborting.\n");
      exitval = EXIT_FAILURE;
      goto EXIT;
    }

    elfu_mToElf(me, hOut.e);

    if (elf_update(hOut.e, ELF_C_WRITE) < 0) {
      fprintf(stderr, "elf_update() failed: %s\n", elf_errmsg(-1));
    }
    closeElf(&hOut);
  }



EXIT:
  if (hIn.e) {
    closeElf(&hIn);
  }

  return (exitval);
}
