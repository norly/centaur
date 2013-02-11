#include <stdio.h>

#include <libelf.h>
#include <gelf.h>

void elfu_fixupPhdrSelfRef(Elf *e)
{
  GElf_Ehdr ehdr;
  size_t i, n;

  if (!gelf_getehdr(e, &ehdr)) {
    fprintf(stderr, "gelf_getehdr() failed: %s.", elf_errmsg(-1));
    return;
  }

  if (elf_getphdrnum(e, &n)) {
    fprintf(stderr, "elf_getphdrnum() failed: %s\n", elf_errmsg(-1));
  }

  for (i = 0; i < n; i++) {
    GElf_Phdr phdr;

    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      fprintf(stderr, "gelf_getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
      continue;
    }

    if (phdr.p_type == PT_PHDR) {
      phdr.p_offset = ehdr.e_phoff;
      phdr.p_filesz = elf32_fsize(ELF_T_PHDR, n, EV_CURRENT);
      phdr.p_memsz = phdr.p_filesz;

      if (!gelf_update_phdr (e, i, &phdr)) {
        fprintf(stderr, "gelf_update_ehdr() failed: %s\n", elf_errmsg(-1));
      }
    }
  }

  /* Tell libelf that phdrs have changed */
  elf_flagphdr(e, ELF_C_SET, ELF_F_DIRTY);
}
