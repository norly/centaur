#ifndef __PRINTING_H__
#define __PRINTING_H__

#include <libelf/libelf.h>


void printHeader(Elf *e);

void printSegmentsWithSection(Elf *e, Elf_Scn *scn);
void printSection(Elf *e, Elf_Scn *scn);
void printSections(Elf *e);

char* segmentTypeStr(size_t pt);
void printSectionsInSegment(Elf *e, GElf_Phdr *phdr);
void printSegments(Elf *e);


#endif
