#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "options.h"


static void printUsage(char *progname)
{
  printf("Usage: %s [OPTIONS] <elf-file>\n", progname);
  printf("\n"
          "Options:\n"
          "  -h, --help                     Print this help message\n"
          "  -o, --output                   Where to write the modified ELF file to\n"
          "\n"
          "ELF dump:\n"
          "      --print-header             Print ELF header\n"
          "      --print-segments           Print program headers\n"
          "      --print-sections           Print sections\n"
          "\n"
          "Space insertion:\n"
          "    off: File offset, not within any structure (headers or sections).\n"
          "    sz:  A multiple of the maximum alignment of all PHDRs.\n"
          "\n"
          "      --expand-nobits off        Expand virtual areas (NOBITS sections and similar PHDRs).\n"
          "      --insert-before off,sz     Insert spacing at given offset,\n"
          "                                 mapping everything before it to lower mem addresses.\n"
          "      --insert-after  off,sz     Insert spacing at given offset,\n"
          "                                 mapping everything after it to higher mem addresses.\n"
          "\n"
          "High-level insertion:\n"
          "      --reladd        obj.o      Automatically insert object file contents\n"
          "\n");
}



void parseOptions(CLIOpts *opts, int argc, char **argv)
{
  char *progname = argv[0];
  int c;
  int option_index = 0;
  char *endptr;

  static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"output", 1, 0, 'o'},
    {"print-header", 0, 0, 10001},
    {"print-segments", 0, 0, 10002},
    {"print-sections", 0, 0, 10003},
    {"insert-before", 1, 0, 10004},
    {"insert-after", 1, 0, 10005},
    {"expand-nobits", 1, 0, 10006},
    {"reladd", 1, 0, 10007},
    {NULL, 0, NULL, 0}
  };

  while ((c = getopt_long(argc, argv, "e:ho:",
                          long_options, &option_index)) != -1) {
    switch (c) {
      case 'h':
        printUsage(progname);
        exit(EXIT_SUCCESS);
      case 'o':
        opts->fnOutput = optarg;
        break;
      case 10001:
        opts->printHeader = 1;
        break;
      case 10002:
        opts->printSegments = 1;
        break;
      case 10003:
        opts->printSections = 1;
        break;
      case 10004:
        opts->insertBeforeOffs = strtoul(optarg, &endptr, 0);
        if (endptr[0] != ',') {
          goto USAGE;
        }
        opts->insertBeforeSz = strtoul(endptr + 1, &endptr, 0);
        if (endptr[0] != '\0' || opts->insertBeforeSz == 0) {
          goto USAGE;
        }
        break;
      case 10005:
        opts->insertAfterOffs = strtoul(optarg, &endptr, 0);
        if (endptr[0] != ',') {
          goto USAGE;
        }
        opts->insertAfterSz = strtoul(endptr + 1, &endptr, 0);
        if (endptr[0] != '\0' || opts->insertAfterSz == 0) {
          goto USAGE;
        }
        break;
      case 10006:
        opts->expandNobitsOffs = strtoul(optarg, &endptr, 0);
        if (endptr[0] != '\0' || opts->expandNobitsOffs < 1) {
          goto USAGE;
        }
        break;
      case 10007:
        opts->fnReladd = optarg;
        break;
      case '?':
      default:
        goto USAGE;
    }
  }

  while (optind < argc) {
    if (!opts->fnInput) {
      opts->fnInput = argv[optind];
    }
    optind++;
  }

  if (!opts->fnInput) {
    fprintf(stderr, "Error: No input file specified.\n\n");
    goto USAGE;
  }

  return;

USAGE:
  printUsage(progname);
  exit(1);
}
