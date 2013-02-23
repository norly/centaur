#include <stdio.h>
#include <stdlib.h>

#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>


/*
 * Returns the first section that  is contained in the segment and
 * ends as close to its memory image of as possible (the "last"
 * section in the segment).
 *
 * If no section fits, NULL is returned.
 */
Elf_Scn* elfu_lastSectionInSegment(Elf *e, GElf_Phdr *phdr)
{
  Elf_Scn *last = NULL;
  Elf_Scn *scn;


  scn = elf_getscn(e, 1);
  while (scn) {
    GElf_Shdr shdr;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      fprintf(stderr, "gelf_getshdr() failed: %s\n", elf_errmsg(-1));
      continue;
    }

    if (elfu_segmentContainsSection(phdr, &shdr)) {
      if (!last) {
        last = scn;
      } else {
        GElf_Shdr shdrOld;

        if (gelf_getshdr(last, &shdrOld) != &shdrOld) {
          continue;
        }

        if (shdr.sh_offset + shdr.sh_size
            > shdrOld.sh_offset + shdrOld.sh_size) {
          // TODO: Check (leftover space in memory image) < (p_align)
          last = scn;
        }
      }
    }

    scn = elf_nextscn(e, scn);
  }

  return last;
}
