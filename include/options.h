#ifndef __OPTIONS_H__
#define __OPTIONS_H__


typedef struct {
  char *fnInput;
  char *fnOutput;
  int printHeader;
  int printSegments;
  int printSections;
  unsigned insertBeforeOffs;
  unsigned insertBeforeSz;
} CLIOpts;


void parseOptions(CLIOpts *opts, int argc, char **argv);

#endif
