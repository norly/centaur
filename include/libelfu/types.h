#ifndef __LIBELFU_TYPES_H__
#define __LIBELFU_TYPES_H__

#include <sys/queue.h>

#include <elf.h>
#include <libelf/gelf.h>


typedef struct ElfuScn {
  GElf_Shdr shdr;

  Elf_Data data;

  struct ElfuScn *linkptr;
  struct ElfuScn *infoptr;

  struct ElfuScn *oldptr;

  CIRCLEQ_ENTRY(ElfuScn) elemChildScn;
  CIRCLEQ_ENTRY(ElfuScn) elem;
} ElfuScn;


typedef struct ElfuPhdr {
  GElf_Phdr phdr;

  CIRCLEQ_HEAD(ChildScnList, ElfuScn) childScnList;
  CIRCLEQ_HEAD(ChildPhdrList, ElfuPhdr) childPhdrList;

  CIRCLEQ_ENTRY(ElfuPhdr) elemChildPhdr;
  CIRCLEQ_ENTRY(ElfuPhdr) elem;
} ElfuPhdr;


typedef struct {
  int elfclass;
  GElf_Ehdr ehdr;

  CIRCLEQ_HEAD(PhdrList, ElfuPhdr) phdrList;
  CIRCLEQ_HEAD(OrphanScnList, ElfuScn) orphanScnList;

  ElfuScn *shstrtab;
} ElfuElf;


#endif
