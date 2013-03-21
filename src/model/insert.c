#include <assert.h>
#include <sys/types.h>
#include <gelf.h>
#include <libelfu/libelfu.h>


// TODO: Take p_align into account



/*
 * Insert space at a given position in the file by moving everything
 * after it towards the end of the file, and everything before it
 * towards lower memory regions where it is mapped.
 *
 * off must not be in the middle of any data structure, such as
 * PHDRs, SHDRs, or sections. Behaviour is undefined if it is.
 *
 * PHDRs will be patched such that everything AFTER off is mapped to
 * the same address in memory, and everything BEFORE it is shifted to
 * lower addresses, making space for the new data in-between.
 */
GElf_Xword elfu_mInsertBefore(ElfuElf *me, GElf_Off off, GElf_Xword size)
{
  ElfuScn *ms;
  ElfuPhdr *mp;

  assert(me);

  /* Move SHDRs and PHDRs */
  if (me->ehdr.e_shoff >= off) {
    me->ehdr.e_shoff += size;
  }

  if (me->ehdr.e_phoff >= off) {
    me->ehdr.e_phoff += size;
  }

  /* Patch PHDRs to include new data */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    GElf_Off end = mp->phdr.p_offset + mp->phdr.p_filesz;

    if (mp->phdr.p_offset >= off) {
      /* Insertion before PHDR's content, so it's just shifted */
      mp->phdr.p_offset += size;
    } else {
      /* mp->phdr.p_offset < off */

      if (off < end) {
        /* Insertion in the middle of PHDR, so let it span the new data */
        mp->phdr.p_filesz += size;
        mp->phdr.p_memsz += size;
        mp->phdr.p_vaddr -= size;
        mp->phdr.p_paddr -= size;
      } else {
        /* Insertion after PHDR's content, so it may need to be
           remapped. This will happen in a second pass.
         */
      }
    }
  }

  /* For each LOAD header, find clashing headers that need to be
     remapped to lower memory areas.
   */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type == PT_LOAD) {
      ElfuPhdr *mp2;

      CIRCLEQ_FOREACH(mp2, &me->phdrList, elem) {
        if (mp2->phdr.p_type != PT_LOAD
            && mp2->phdr.p_offset + mp2->phdr.p_filesz <= off) {
          /* The PHDR ends in the file before the injection site */
          GElf_Off vend1 = mp->phdr.p_vaddr + mp->phdr.p_memsz;
          GElf_Off pend1 = mp->phdr.p_paddr + mp->phdr.p_memsz;
          GElf_Off vend2 = mp2->phdr.p_vaddr + mp2->phdr.p_memsz;
          GElf_Off pend2 = mp2->phdr.p_paddr + mp2->phdr.p_memsz;

          /* If mp and mp2 now overlap in memory */
          if ((mp2->phdr.p_vaddr < vend1 && vend2 > mp->phdr.p_vaddr)
              || (mp2->phdr.p_paddr < pend1 && pend2 > mp->phdr.p_paddr)) {
            /* Move mp2 down in memory, as mp has been resized.
               Maintaining the relative offset between them is the best
               guess at maintaining consistency.
             */
            mp2->phdr.p_vaddr -= size;
            mp2->phdr.p_paddr -= size;
          }
        }
      }
    }
  }

  /* Move the sections themselves */
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if (ms->shdr.sh_offset >= off) {
      ms->shdr.sh_offset += size;
    } else {
      /* sh_offset < off */

      /* If this was in a LOAD segment, it has been adjusted there
         and this synchronises it.
         If not, it doesn't matter anyway.
       */
      ms->shdr.sh_addr -= size;
    }
  }

  return size;
}



/*
 * Insert space at a given position in the file by moving everything
 * after it towards the end of the file, and towards higher memory
 * regions where it is mapped.
 *
 * off must not be in the middle of any data structure, such as
 * PHDRs, SHDRs, or sections. Behaviour is undefined if it is.
 *
 * PHDRs will be patched such that everything AFTER off is shifted to
 * higher addresses, making space for the new data in-between.
 *
 * CAUTION: This also moves NOBITS sections. If such are present,
 *          use mExpandNobits() first and then inject at the end of
 *          the expansion site.
 */
GElf_Xword elfu_mInsertAfter(ElfuElf *me, GElf_Off off, GElf_Xword size)
{
  ElfuScn *ms;
  ElfuPhdr *mp;

  assert(me);

/* Move SHDRs and PHDRs */
  if (me->ehdr.e_shoff >= off) {
    me->ehdr.e_shoff += size;
  }

  if (me->ehdr.e_phoff >= off) {
    me->ehdr.e_phoff += size;
  }

  /* Patch PHDRs to include new data */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    GElf_Off end = mp->phdr.p_offset + mp->phdr.p_filesz;

    if (mp->phdr.p_offset >= off) {
      /* Insertion before PHDR's content, so it's shifted. */
      mp->phdr.p_offset += size;

      /* It may also need to be remapped. See second pass below. */
    } else {
      /* mp->phdr.p_offset < off */

      if (off < end) {
        /* Insertion in the middle of PHDR, so let it span the new data */
        mp->phdr.p_filesz += size;
        mp->phdr.p_memsz += size;
      } else {
        /* Insertion after PHDR's content. Nothing to do. */
      }
    }
  }

  /* For each LOAD header, find clashing headers that need to be
     remapped to higher memory areas.
   */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type == PT_LOAD) {
      ElfuPhdr *mp2;

      CIRCLEQ_FOREACH(mp2, &me->phdrList, elem) {
        if (mp2->phdr.p_type != PT_LOAD
            && mp2->phdr.p_offset + mp2->phdr.p_filesz > off) {
          /* The PHDR now ends in the file after the injection site */
          GElf_Off vend1 = mp->phdr.p_vaddr + mp->phdr.p_memsz;
          GElf_Off pend1 = mp->phdr.p_paddr + mp->phdr.p_memsz;
          GElf_Off vend2 = mp2->phdr.p_vaddr + mp2->phdr.p_memsz;
          GElf_Off pend2 = mp2->phdr.p_paddr + mp2->phdr.p_memsz;

          /* If mp and mp2 now overlap in memory */
          if ((mp2->phdr.p_vaddr < vend1 && vend2 > mp->phdr.p_vaddr)
              || (mp2->phdr.p_paddr < pend1 && pend2 > mp->phdr.p_paddr)) {
            /* Move mp2 up in memory, as mp has been resized.
               Maintaining the relative offset between them is the best
               guess at maintaining consistency.
             */

            mp2->phdr.p_vaddr += size;
            mp2->phdr.p_paddr += size;
          }
        }
      }
    }
  }

  /* Move the sections themselves */
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if (ms->shdr.sh_offset >= off) {
      ms->shdr.sh_offset += size;

      /* If this was in a LOAD segment, it has been adjusted there
         and this synchronises it.
         If not, it doesn't matter anyway.
       */
      ms->shdr.sh_addr += size;
    }
  }

  return size;
}
