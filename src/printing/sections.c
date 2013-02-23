#include <stdio.h>

#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>
#include "printing.h"


void printSegmentsWithSection(Elf *e, Elf_Scn *scn)
{
  GElf_Phdr phdr;
  GElf_Shdr shdr;
  int i;
  size_t n;


  if (gelf_getshdr(scn, &shdr) != &shdr) {
    fprintf(stderr, "gelf_getshdr() failed: %s\n", elf_errmsg(-1));
    return;
  }

  if (elf_getphdrnum(e, &n)) {
    fprintf(stderr, "elf_getphdrnum() failed: %s\n", elf_errmsg(-1));
    return;
  }

  for (i = 0; i < n; i++) {
    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      fprintf(stderr, "getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
      continue;
    }

    if (elfu_ePhdrContainsScn(&phdr, &shdr)) {
      printf("     %d %s\n", i, segmentTypeStr(phdr.p_type));
    }
  }
}


void printSection(Elf *e, Elf_Scn *scn)
{
  printf("  %jd: %s\n",
          (uintmax_t) elf_ndxscn(scn),
          elfu_eScnName(e, scn));
}


void printSections(Elf *e)
{
  Elf_Scn *scn;

  printf("Sections:\n");

  scn = elf_getscn(e, 0);

  while (scn) {
    printSection(e, scn);
    //printSegmentsWithSection(e, scn);

    scn = elf_nextscn(e, scn);
  }
}
