#include <stdio.h>

#include <libelf.h>
#include <gelf.h>


void printHeader(Elf *e)
{
  GElf_Ehdr ehdr;
  int elfclass;
  size_t shdrnum, shdrstrndx, phdrnum;


  printf("ELF header:\n");


  elfclass = gelf_getclass(e);
  if (elfclass == ELFCLASSNONE) {
    fprintf(stderr, "getclass() failed: %s\n", elf_errmsg(-1));
  }
  printf(" * %d-bit ELF object\n", elfclass == ELFCLASS32 ? 32 : 64);


  if (!gelf_getehdr(e, &ehdr)) {
    fprintf(stderr, "getehdr() failed: %s.", elf_errmsg(-1));
    return;
  }

  printf(" *     type  machine  version    entry    phoff    shoff    flags   ehsize phentsize shentsize\n");
  printf("   %8jx %8jx %8jx %8jx %8jx %8jx %8jx %8jx %9jx %9jx\n",
          (uintmax_t) ehdr.e_type,
          (uintmax_t) ehdr.e_machine,
          (uintmax_t) ehdr.e_version,
          (uintmax_t) ehdr.e_entry,
          (uintmax_t) ehdr.e_phoff,
          (uintmax_t) ehdr.e_shoff,
          (uintmax_t) ehdr.e_flags,
          (uintmax_t) ehdr.e_ehsize,
          (uintmax_t) ehdr.e_phentsize,
          (uintmax_t) ehdr.e_shentsize);


  if (elf_getshdrnum(e, &shdrnum) != 0) {
    fprintf(stderr, "getshdrnum() failed: %s\n", elf_errmsg(-1));
  } else {
    printf(" * (shnum):    %u\n", shdrnum);
  }

  if (elf_getshdrstrndx(e, &shdrstrndx) != 0) {
    fprintf(stderr, "getshdrstrndx() failed: %s\n", elf_errmsg(-1));
  } else {
    printf(" * (shstrndx): %u\n", shdrstrndx);
  }

  if (elf_getphdrnum(e, &phdrnum) != 0) {
    fprintf(stderr, "getphdrnum() failed: %s\n", elf_errmsg(-1));
  } else {
    printf(" * (phnum):    %u\n", phdrnum);
  }

  printf("\n");
}
