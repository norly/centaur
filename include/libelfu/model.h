#ifndef __LIBELFU_MODEL_H__
#define __LIBELFU_MODEL_H__

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



size_t elfu_countSections(ElfuElf *me);
size_t elfu_countPHDRs(ElfuElf *me);

char* elfu_modelScnName(ElfuElf *me, ElfuScn *ms);

int elfu_modelCheck(ElfuElf *me);

ElfuPhdr* elfu_modelFromPhdr(GElf_Phdr *phdr);
ElfuScn* elfu_modelFromSection(Elf_Scn *scn);
ElfuElf* elfu_modelFromElf(Elf *e);

void elfu_modelToElf(ElfuElf *me, Elf *e);

#endif