#ifndef __LIBELFU_TYPES_H__
#define __LIBELFU_TYPES_H__

#include <sys/queue.h>

#include <elf.h>
#include <gelf.h>


typedef struct ElfuSym {
  GElf_Word name;
  char *nameptr;

  GElf_Addr value;
  GElf_Word size;

  unsigned char bind;
  unsigned char type;
  unsigned char other;

  struct ElfuScn *scnptr;
  int shndx;

  CIRCLEQ_ENTRY(ElfuSym) elem;
} ElfuSym;


typedef struct ElfuSymtab {
  CIRCLEQ_HEAD(Syms, ElfuSym) syms;
} ElfuSymtab;



typedef struct ElfuRel {
  char *name;

  GElf_Addr offset;
  GElf_Word info;

  GElf_Word sym;
  unsigned char type;

  int addendUsed;
  GElf_Sword addend;

  CIRCLEQ_ENTRY(ElfuRel) elem;
} ElfuRel;


typedef struct ElfuReltab {
  CIRCLEQ_HEAD(Rels, ElfuRel) rels;
} ElfuReltab;






typedef struct ElfuScn {
  GElf_Shdr shdr;

  Elf_Data data;

  struct ElfuScn *linkptr;
  struct ElfuScn *infoptr;

  struct ElfuScn *oldptr;

  struct ElfuSymtab symtab;
  struct ElfuReltab reltab;

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

  ElfuScn *symtab;
} ElfuElf;

#endif
