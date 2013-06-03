#include <libelfu/libelfu.h>


void elfu_ePhdrFixupSelfRef(Elf *e)
{
  GElf_Ehdr ehdr;
  size_t i, n;

  if (!gelf_getehdr(e, &ehdr)) {
    ELFU_WARNELF("gelf_getehdr");
    return;
  }

  if (elf_getphdrnum(e, &n)) {
    ELFU_WARNELF("elf_getphdrnum");
  }

  for (i = 0; i < n; i++) {
    GElf_Phdr phdr;

    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      ELFU_WARN("gelf_getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
      continue;
    }

    if (phdr.p_type == PT_PHDR) {
      phdr.p_offset = ehdr.e_phoff;
      phdr.p_filesz = elf32_fsize(ELF_T_PHDR, n, EV_CURRENT);
      phdr.p_memsz = phdr.p_filesz;

      if (!gelf_update_phdr (e, i, &phdr)) {
        ELFU_WARNELF("gelf_update_ehdr");
      }
    }
  }

  /* Tell libelf that phdrs have changed */
  elf_flagphdr(e, ELF_C_SET, ELF_F_DIRTY);
}
