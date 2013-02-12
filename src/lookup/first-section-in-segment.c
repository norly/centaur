#include <stdlib.h>

#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>


/*
 * Returns the section that starts at the same point in the file as
 * the segment AND is wholly contained in the memory image.
 *
 * If no section fits, NULL is returned.
 */
Elf_Scn* elfu_firstSectionInSegment(Elf *e, GElf_Phdr *phdr)
{
  Elf_Scn *scn;

  scn = elf_getscn(e, 1);
  while (scn) {
    GElf_Shdr shdr;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      return NULL;
    }

    if (shdr.sh_offset == phdr->p_offset
        && elfu_segmentContainsSection(phdr, scn) == ELFU_TRUE) {
      return scn;
    }

    scn = elf_nextscn(e, scn);
  }

  return NULL;
}
