#include <string.h>

#include <libelf/libelf.h>
#include <libelf/gelf.h>

#include <libelfu/libelfu.h>


Elf_Scn* elfu_eScnByName(Elf *e, char *name)
{
  size_t shstrndx;
  Elf_Scn *scn;

  if (elf_getshdrstrndx(e, &shstrndx) != 0) {
    return NULL;
  }

  scn = elf_getscn(e, 1);
  while (scn) {
    GElf_Shdr shdr;
    char *curname;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      return NULL;
    }

    /* elf_strptr returns NULL if there was an error */
    curname = elf_strptr(e, shstrndx, shdr.sh_name);

    /* strcmp... but we really have no bounds on the lengths here */
    if (!strcmp(curname, name)) {
      return scn;
    }

    scn = elf_nextscn(e, scn);
  }

  return NULL;
}
