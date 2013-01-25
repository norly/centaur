#ifndef __LIBELFU_LOOKUP_H_
#define __LIBELFU_LOOKUP_H_

#include <libelf.h>
#include <gelf.h>

#include <libelfu/types.h>

char* elfu_sectionName(Elf *e, Elf_Scn *scn);
Elf_Scn* elfu_sectionByName(Elf *e, char *name);
Elf_Scn* elfu_firstSectionInSegment(Elf *e, GElf_Phdr *phdr);
Elf_Scn* elfu_lastSectionInSegment(Elf *e, GElf_Phdr *phdr);

#endif
