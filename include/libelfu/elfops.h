#ifndef __LIBELFU_ELFOPS_H_
#define __LIBELFU_ELFOPS_H_

#include <libelf.h>
#include <gelf.h>

#include <libelfu/types.h>

size_t elfu_scnSizeFile(const GElf_Shdr *shdr);

char* elfu_sectionName(Elf *e, Elf_Scn *scn);
Elf_Scn* elfu_sectionByName(Elf *e, char *name);

int elfu_segmentContainsSection(GElf_Phdr *phdr, GElf_Shdr *shdr);
Elf_Scn* elfu_firstSectionInSegment(Elf *e, GElf_Phdr *phdr);
Elf_Scn* elfu_lastSectionInSegment(Elf *e, GElf_Phdr *phdr);

void elfu_fixupPhdrSelfRef(Elf *e);

#endif
