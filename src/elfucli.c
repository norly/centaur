#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libelfu/libelfu.h>

#include "elfhandle.h"



static void printUsage(char *progname)
{
  printf("Usage: %s -i <inputfile> [OPTIONS]\n", progname);
  printf("\n"
         "Options, executed in order given:\n"
         "  -h, --help                     Print this help message\n"
         "\n"
         "  -i, --input         infile     Load new ELF file (must be first command)\n"
         "\n"
         "  -c, --check                    Do a few sanity checks and print any errors\n"
         "  -d, --dump                     Dump current model state (debug only)\n"
         "      --reladd        obj.o      Insert object file contents\n"
         "      --detour        from,to    Write a jump to <to> at <from>\n"
         "  -o, --output        outfile    Where to write the modified ELF file to\n"
         "\n");
}




int main(int argc, char **argv)
{
  ELFHandles hIn = { 0 };
  int exitval = EXIT_SUCCESS;
  char *progname = argv[0];
  int c;
  int option_index = 0;
  ElfuElf *me = NULL;

  static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"check", 0, 0, 'c'},
    {"dump", 0, 0, 'd'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {"reladd", 1, 0, 10001},
    {"detour", 1, 0, 10002},
    {NULL, 0, NULL, 0}
  };


  if (argc < 3) {
    printUsage(progname);
    goto EXIT;
  }


  /* Is libelf alive and well? */
  if (elf_version(EV_CURRENT) == EV_NONE) {
    fprintf(stderr, "libelf init error: %s\n", elf_errmsg(-1));
  }


  /* Parse and and execute user commands */
  while ((c = getopt_long(argc, argv, "hcdi:o:",
                          long_options, &option_index)) != -1) {
    switch (c) {
      case 'h':
        printUsage(progname);
        goto EXIT;
      case 'c':
        if (!me) {
          goto ERR_NO_INPUT;
        } else {
          printf("Checking model validity...\n");
          elfu_mCheck(me);
        }
        break;
      case 'd':
        if (!me) {
          goto ERR_NO_INPUT;
        } else {
          elfu_mDumpElf(me);
        }
        break;
      case 'i':
        printf("Opening input file %s.\n", optarg);
        openElf(&hIn, optarg, ELF_C_READ);
        if (!hIn.e) {
          printf("Error: Failed to open input file. Aborting.\n");
          exitval = EXIT_FAILURE;
          goto EXIT;
        }

        me = elfu_mFromElf(hIn.e);
        closeElf(&hIn);

        if (!me) {
          printf("Error: Failed to load model, aborting.\n");
          goto EXIT;
        }
        break;
      case 'o':
        if (!me) {
          goto ERR_NO_INPUT;
        } else {
          ELFHandles hOut = { 0 };

          printf("Writing modified file to %s.\n", optarg);

          openElf(&hOut, optarg, ELF_C_WRITE);
          if (!hOut.e) {
            printf("Failed to open output file. Aborting.\n");
            closeElf(&hOut);
            exitval = EXIT_FAILURE;
            goto EXIT;
          }

          elfu_mToElf(me, hOut.e);

          if (elf_update(hOut.e, ELF_C_WRITE) < 0) {
            fprintf(stderr, "elf_update() failed: %s\n", elf_errmsg(-1));
          }
          closeElf(&hOut);
        }
        break;
      case 10001:
        if (!me) {
          goto ERR_NO_INPUT;
        } else {
          ELFHandles hRel = { 0 };
          ElfuElf *mrel = NULL;

          openElf(&hRel, optarg, ELF_C_READ);
          if (!hRel.e) {
            printf("--reladd: Failed to open file for --reladd, aborting.\n");
            closeElf(&hRel);
            goto ERR;
          }

          mrel = elfu_mFromElf(hRel.e);
          closeElf(&hRel);
          if (!mrel) {
            printf("--reladd: Failed to load model for --reladd, aborting.\n");
            goto ERR;
          } else {
            printf("--reladd: Injecting %s...\n", optarg);
            elfu_mCheck(mrel);
            elfu_mReladd(me, mrel);
            printf("--reladd: Injected %s.\n", optarg);
          }
        }
        break;
      case 10002:
        if (!me) {
          goto ERR_NO_INPUT;
        } else {
          GElf_Addr from;
          GElf_Addr to;
          char *second;

          strtok_r(optarg, ",", &second);
          printf("--detour: From '%s' to '%s'\n", optarg, second);


          from = strtoul(optarg, NULL, 0);
          if (from == 0) {
            from = elfu_mSymtabLookupAddrByName(me, me->symtab, optarg);
          }
          if (from == 0) {
            printf("--detour: Cannot parse argument 1, aborting.\n");
            goto ERR;
          }
          printf("--detour: From %x\n", (unsigned)from);

          to = strtoul(second, NULL, 0);
          if (to == 0) {
            to = elfu_mSymtabLookupAddrByName(me, me->symtab, second);
          }
          if (to == 0) {
            printf("--detour: Cannot parse argument 2, aborting.\n");
            goto ERR;
          }
          printf("--detour: To %x\n", (unsigned)to);

          elfu_mDetour(me, from, to);
        }
        break;
      case '?':
      default:
        printUsage(progname);
        goto EXIT;
    }
  }

  while (optind < argc) {
    optind++;
  }



EXIT:
  if (hIn.e) {
    closeElf(&hIn);
  }

  return (exitval);


ERR_NO_INPUT:
  printf("Error: No input file opened. Aborting.\n");

ERR:
  exitval = EXIT_FAILURE;
  goto EXIT;
}
