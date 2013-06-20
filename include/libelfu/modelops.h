#ifndef __LIBELFU_MODELOPS_H__
#define __LIBELFU_MODELOPS_H__

#include <elf.h>
#include <gelf.h>

#include <libelfu/types.h>


#define ELFU_SYMSTR(symtabscn, off) ((symtabscn)->linkptr->databuf + (off))


GElf_Word elfu_mSymtabLookupVal(ElfuElf *me, ElfuScn *msst, GElf_Word entry);
GElf_Word elfu_mSymtabLookupAddrByName(ElfuElf *me, ElfuScn *msst, char *name);
void elfu_mSymtabFlatten(ElfuElf *me);


void elfu_mRelocate(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt);


   size_t elfu_mPhdrCount(ElfuElf *me);
     void elfu_mPhdrUpdateChildOffsets(ElfuPhdr *mp);
ElfuPhdr* elfu_mPhdrAlloc();
     void elfu_mPhdrDestroy(ElfuPhdr* mp);


typedef void* (SectionHandlerFunc)(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2);
    void* elfu_mScnForall(ElfuElf *me, SectionHandlerFunc f, void *aux1, void *aux2);
   size_t elfu_mScnCount(ElfuElf *me);
   size_t elfu_mScnIndex(ElfuElf *me, ElfuScn *ms);
 ElfuScn* elfu_mScnByOldscn(ElfuElf *me, ElfuScn *oldscn);
    char* elfu_mScnName(ElfuElf *me, ElfuScn *ms);
ElfuScn** elfu_mScnSortedByOffset(ElfuElf *me, size_t *count);
 ElfuScn* elfu_mScnAlloc();
     void elfu_mScnDestroy(ElfuScn* ms);


ElfuElf* elfu_mElfAlloc();
    void elfu_mElfDestroy(ElfuElf* me);


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

void elfu_mDetour(ElfuElf *me, GElf_Addr from, GElf_Addr to);

#endif
