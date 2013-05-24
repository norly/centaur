#include <assert.h>
#include <sys/types.h>
#include <libelf/gelf.h>
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
GElf_Xword elfu_mInsertSpaceBefore(ElfuElf *me, GElf_Off off, GElf_Xword size)
{
  ElfuScn *ms;
  ElfuPhdr *mp;

  assert(me);

  /* Round up size to 4096 bytes to keep page alignment on x86 when
   * remapping existing data to lower addresses. */
  size += (4096 - (size % 4096)) % 4096;
  // TODO: Find alignment size by checking p_align in PHDRs

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
GElf_Xword elfu_mInsertSpaceAfter(ElfuElf *me, GElf_Off off, GElf_Xword size)
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





/* Update cross-references */
static void shiftSections(ElfuElf *me, ElfuScn *first)
{
  ElfuScn *ms = first;
  size_t firstIndex = elfu_mScnIndex(me, first);

  do {
    if (ms == me->shstrtab) {
      me->ehdr.e_shstrndx++;
    }

    ms = CIRCLEQ_LOOP_NEXT(&me->scnList, ms, elem);
  } while (ms != CIRCLEQ_FIRST(&me->scnList));

  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    switch (ms->shdr.sh_type) {
      case SHT_REL:
      case SHT_RELA:
        if (ms->shdr.sh_info >= firstIndex) {
          ms->shdr.sh_info++;
        }
      case SHT_DYNAMIC:
      case SHT_HASH:
      case SHT_SYMTAB:
      case SHT_DYNSYM:
      case SHT_GNU_versym:
      case SHT_GNU_verdef:
      case SHT_GNU_verneed:
        if (ms->shdr.sh_link >= firstIndex) {
          ms->shdr.sh_link++;
        }
    }
  }
}


/*
 * Insert a section into an ELF model, /before/ a given other section
 */
void elfu_mInsertScnInChainBefore(ElfuElf *me, ElfuScn *oldscn, ElfuScn *newscn)
{
  assert(me);
  assert(oldscn);
  assert(newscn);

  shiftSections(me, oldscn);

  CIRCLEQ_INSERT_BEFORE(&me->scnList, oldscn, newscn, elem);
}


/*
 * Insert a section into an ELF model, /after/ a given other section
 */
void elfu_mInsertScnInChainAfter(ElfuElf *me, ElfuScn *oldscn, ElfuScn *newscn)
{
  assert(me);
  assert(oldscn);
  assert(newscn);

  if (oldscn != CIRCLEQ_LAST(&me->scnList)) {
    shiftSections(me, CIRCLEQ_NEXT(oldscn, elem));
  }

  CIRCLEQ_INSERT_AFTER(&me->scnList, oldscn, newscn, elem);
}
