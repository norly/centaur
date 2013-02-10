#ifndef __OPTIONS_H__
#define __OPTIONS_H__


typedef struct {
  char *fnInput;
  char *fnOutput;
  int printHeader;
  int printSegments;
  int printSections;
  int copy;
} CLIOpts;


void parseOptions(CLIOpts *opts, int argc, char **argv);

#endif
