#ifndef __LIBELFU_MODELOPS_H__
#define __LIBELFU_MODELOPS_H__

#include <elf.h>
#include <gelf.h>

#include <libelfu/modeltypes.h>


size_t elfu_mCountScns(ElfuElf *me);
size_t elfu_mCountPhdrs(ElfuElf *me);

char* elfu_mScnName(ElfuElf *me, ElfuScn *ms);

int elfu_mCheck(ElfuElf *me);

ElfuElf* elfu_mFromElf(Elf *e);
    void elfu_mToElf(ElfuElf *me, Elf *e);


GElf_Xword elfu_mInsertBefore(ElfuElf *me, GElf_Off off, GElf_Xword size);
GElf_Xword elfu_mInsertAfter(ElfuElf *me, GElf_Off off, GElf_Xword size);

#endif
