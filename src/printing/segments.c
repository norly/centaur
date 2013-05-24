#include <stdio.h>

#include <libelf/libelf.h>
#include <libelf/gelf.h>

#include <libelfu/libelfu.h>


static char *ptstr[] = {"NULL", "LOAD", "DYNAMIC", "INTERP", "NOTE", "SHLIB", "PHDR", "TLS", "NUM"};


char* segmentTypeStr(size_t pt)
{
  if (pt >= 0 && pt <= PT_NUM) {
    return ptstr[pt];
  }

  return "-?-";
}


void printSectionsInSegment(Elf *e, GElf_Phdr *phdr)
{
  Elf_Scn *scn;

  scn = elf_getscn(e, 0);

  while (scn) {
    GElf_Shdr shdr;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      fprintf(stderr, "gelf_getshdr() failed: %s\n", elf_errmsg(-1));
      continue;
    }

    if (elfu_gPhdrContainsScn(phdr, &shdr)) {
      printf("       %10u %s\n", elf_ndxscn(scn), elfu_eScnName(e, scn));
    }

    scn = elf_nextscn(e, scn);
  }
}


void printSegments(Elf *e)
{
  size_t i, n;

  if (elf_getphdrnum(e, &n)) {
    fprintf(stderr, "elf_getphdrnum() failed: %s.", elf_errmsg(-1));
  }

  printf("Segments:\n");
  printf("      #   typeStr     type   offset    vaddr    paddr   filesz    memsz    flags    align\n");

  for (i = 0; i < n; i++) {
    GElf_Phdr phdr;

    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      fprintf(stderr, "getphdr() failed for #%d: %s.", i, elf_errmsg(-1));
      continue;
    }

    printf(" * %4d: %8s ", i, segmentTypeStr(phdr.p_type));

    printf("%8jx %8jx %8jx %8jx %8jx %8jx %8jx %8jx\n",
            (uintmax_t) phdr.p_type,
            (uintmax_t) phdr.p_offset,
            (uintmax_t) phdr.p_vaddr,
            (uintmax_t) phdr.p_paddr,
            (uintmax_t) phdr.p_filesz,
            (uintmax_t) phdr.p_memsz,
            (uintmax_t) phdr.p_flags,
            (uintmax_t) phdr.p_align);

    printSectionsInSegment(e, &phdr);
  }

  printf("\n");
}
