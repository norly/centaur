#include <stdlib.h>
#include <libelfu/libelfu.h>


/*
 * Returns the section that starts at the same point in the file as
 * the segment AND is wholly contained in the memory image.
 *
 * If no section fits, NULL is returned.
 */
Elf_Scn* elfu_eScnFirstInSegment(Elf *e, GElf_Phdr *phdr)
{
  Elf_Scn *scn;

  scn = elf_getscn(e, 1);
  while (scn) {
    GElf_Shdr shdr;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      return NULL;
    }

    if (shdr.sh_offset == phdr->p_offset
        && PHDR_CONTAINS_SCN_IN_MEMORY(phdr, &shdr)) {
      return scn;
    }

    scn = elf_nextscn(e, scn);
  }

  return NULL;
}



/*
 * Returns the first section that  is contained in the segment and
 * ends as close to its memory image of as possible (the "last"
 * section in the segment).
 *
 * If no section fits, NULL is returned.
 */
Elf_Scn* elfu_eScnLastInSegment(Elf *e, GElf_Phdr *phdr)
{
  Elf_Scn *last = NULL;
  Elf_Scn *scn;


  scn = elf_getscn(e, 1);
  while (scn) {
    GElf_Shdr shdr;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      ELFU_WARNELF("gelf_getshdr");
      continue;
    }

    if (PHDR_CONTAINS_SCN_IN_MEMORY(phdr, &shdr)) {
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
