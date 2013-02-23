#include <stdlib.h>

#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>


char* elfu_sectionName(Elf *e, Elf_Scn *scn)
{
  size_t shstrndx;
  GElf_Shdr shdr;

  if (elf_getshdrstrndx(e, &shstrndx) != 0) {
    return NULL;
  }

  if (gelf_getshdr(scn, &shdr) != &shdr) {
    return NULL;
  }

  /* elf_strptr returns NULL if there was an error */
  return elf_strptr(e, shstrndx, shdr.sh_name);
}
