#ifndef __LIBELFU_MODELOPS_H__
#define __LIBELFU_MODELOPS_H__

#include <elf.h>
#include <libelf/gelf.h>

#include <libelfu/modeltypes.h>


size_t elfu_mScnIndex(ElfuElf *me, ElfuScn *ms);
size_t elfu_mCountScns(ElfuElf *me);
size_t elfu_mCountPhdrs(ElfuElf *me);

char* elfu_mScnName(ElfuElf *me, ElfuScn *ms);

ElfuScn* elfu_mScnFirstInSegment(ElfuElf *me, ElfuPhdr *mp);
ElfuScn* elfu_mScnLastInSegment(ElfuElf *me, ElfuPhdr *mp);

ElfuScn* elfu_mScnByType(ElfuElf *me, Elf32_Word type);

int elfu_mCheck(ElfuElf *me);

ElfuScn* elfu_mCloneScn(ElfuScn *ms);


void elfu_mDumpPhdr(ElfuElf *me, ElfuPhdr *mp);
void elfu_mDumpScn(ElfuElf *me, ElfuScn *ms);
void elfu_mDumpElf(ElfuElf *me);


ElfuElf* elfu_mFromElf(Elf *e);
    void elfu_mToElf(ElfuElf *me, Elf *e);


      void elfu_mExpandNobits(ElfuElf *me, GElf_Off off);

GElf_Xword elfu_mInsertSpaceBefore(ElfuElf *me, GElf_Off off, GElf_Xword size);
GElf_Xword elfu_mInsertSpaceAfter(ElfuElf *me, GElf_Off off, GElf_Xword size);

void elfu_mInsertScnInChainBefore(ElfuElf *me, ElfuScn *oldscn, ElfuScn *newscn);
void elfu_mInsertScnInChainAfter(ElfuElf *me, ElfuScn *oldscn, ElfuScn *newscn);


void elfu_mReladd(ElfuElf *me, ElfuElf *mrel);

#endif
