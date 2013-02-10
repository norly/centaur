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
          "  -h, --help                 Print this help message\n"
          "  -o, --output               Where to write the modified ELF file to\n"
          "\n"
          "      --print-header         Print ELF header\n"
          "      --print-segments       Print program headers\n"
          "      --print-sections       Print sections\n"
          "\n");
}



void parseOptions(CLIOpts *opts, int argc, char **argv)
{
  char *progname = argv[0];
  int c;
  int option_index = 0;

  static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"output", 1, 0, 'o'},
    {"print-header", 0, 0, 10001},
    {"print-segments", 0, 0, 10002},
    {"print-sections", 0, 0, 10003},
    {"model", 0, 0, 10004},
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
        opts->model = 1;
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
