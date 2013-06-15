#ifndef __LIBELFU_MODELOPS_H__
#define __LIBELFU_MODELOPS_H__

#include <elf.h>
#include <gelf.h>

#include <libelfu/types.h>


#define ELFU_SYMSTR(symtabscn, off) (((char*)(symtabscn)->linkptr->data.d_buf) + (off))


size_t elfu_mPhdrCount(ElfuElf *me);
void elfu_mPhdrUpdateChildOffsets(ElfuPhdr *mp);


typedef void* (SectionHandlerFunc)(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2);
    void* elfu_mScnForall(ElfuElf *me, SectionHandlerFunc f, void *aux1, void *aux2);
   size_t elfu_mScnCount(ElfuElf *me);
   size_t elfu_mScnIndex(ElfuElf *me, ElfuScn *ms);
 ElfuScn* elfu_mScnByOldscn(ElfuElf *me, ElfuScn *oldscn);
    char* elfu_mScnName(ElfuElf *me, ElfuScn *ms);
ElfuScn** elfu_mScnSortedByOffset(ElfuElf *me, size_t *count);


GElf_Addr elfu_mLayoutGetSpaceInPhdr(ElfuElf *me, GElf_Word size,
                                     GElf_Word align, int w, int x,
                                     ElfuPhdr **injPhdr);
int elfu_mLayoutAuto(ElfuElf *me);


void elfu_mRelocate(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt);


int elfu_mCheck(ElfuElf *me);

ElfuScn* elfu_mCloneScn(ElfuScn *ms);


void elfu_mDumpPhdr(ElfuElf *me, ElfuPhdr *mp);
void elfu_mDumpScn(ElfuElf *me, ElfuScn *ms);
void elfu_mDumpElf(ElfuElf *me);


ElfuElf* elfu_mFromElf(Elf *e);
    void elfu_mToElf(ElfuElf *me, Elf *e);

void elfu_mReladd(ElfuElf *me, const ElfuElf *mrel);

#endif
