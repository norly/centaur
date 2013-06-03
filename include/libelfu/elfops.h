#ifndef __LIBELFU_ELFOPS_H_
#define __LIBELFU_ELFOPS_H_

#include <libelf.h>
#include <gelf.h>

#include <libelfu/types.h>


int elfu_eCheck(Elf *e);

   char* elfu_eScnName(Elf *e, Elf_Scn *scn);
Elf_Scn* elfu_eScnByName(Elf *e, char *name);

Elf_Scn* elfu_eScnFirstInSegment(Elf *e, GElf_Phdr *phdr);
Elf_Scn* elfu_eScnLastInSegment(Elf *e, GElf_Phdr *phdr);

void elfu_ePhdrFixupSelfRef(Elf *e);

#endif
