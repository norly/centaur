#ifndef __LIBELFU_MODELTYPES_H__
#define __LIBELFU_MODELTYPES_H__

#include <sys/queue.h>

#include <elf.h>
#include <libelf/gelf.h>


typedef struct ElfuScn {
  GElf_Shdr shdr;

  Elf_Data data;

  struct ElfuScn *linkptr;
  struct ElfuScn *infoptr;

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

  CIRCLEQ_HEAD(ScnList, ElfuScn) scnList;
  CIRCLEQ_HEAD(PhdrList, ElfuPhdr) phdrList;

  ElfuScn *shstrtab;
} ElfuElf;


#endif
