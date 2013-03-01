#ifndef __LIBELFU_MODELTYPES_H__
#define __LIBELFU_MODELTYPES_H__

#include <sys/queue.h>

#include <elf.h>
#include <gelf.h>

typedef struct ElfuData {
  Elf_Data data;

  CIRCLEQ_ENTRY(ElfuData) elem;
} ElfuData;


typedef struct ElfuScn {
  GElf_Shdr shdr;

  CIRCLEQ_HEAD(DataList, ElfuData) dataList;

  CIRCLEQ_ENTRY(ElfuScn) elem;
} ElfuScn;


typedef struct ElfuPhdr {
  GElf_Phdr phdr;

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