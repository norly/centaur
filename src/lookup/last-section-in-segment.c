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
    if (elfu_segmentContainsSection(phdr, scn) == ELFU_TRUE) {
      if (!last) {
        last = scn;
      } else {
        GElf_Shdr shdrOld;
        GElf_Shdr shdrNew;

        if (gelf_getshdr(last, &shdrOld) != &shdrOld) {
          continue;
        }

        if (gelf_getshdr(scn, &shdrNew) != &shdrNew) {
          continue;
        }

        if (shdrNew.sh_offset + shdrNew.sh_size
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
