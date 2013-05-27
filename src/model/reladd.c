#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>

typedef enum Destsegment {
  DS_UNKNOWN,
  DS_TEXT,
  DS_DATA,
} Destsegment;


static Destsegment destsegment(ElfuScn *ms)
{
  if (!(ms->shdr.sh_flags & SHF_ALLOC)) {
    return DS_UNKNOWN;
  }

  if (!(ms->shdr.sh_flags & SHF_WRITE)
      && (ms->shdr.sh_flags & SHF_EXECINSTR)) {
    return DS_TEXT;
  } else if ((ms->shdr.sh_flags & SHF_WRITE)
             && !(ms->shdr.sh_flags & SHF_EXECINSTR)) {
    return DS_DATA;
  }

  return DS_UNKNOWN;
}



static ElfuScn* insertSection(ElfuElf *me, ElfuElf *mrel, ElfuScn *ms)
{
  ElfuPhdr *mp;
  ElfuPhdr *first = NULL;
  ElfuPhdr *last = NULL;
  ElfuScn *newscn = NULL;
  ElfuPhdr *injAnchor;
  int searchForCode = 0;

  switch (destsegment(ms)) {
    case DS_TEXT:
      searchForCode = 1;
    case DS_DATA:
      newscn = elfu_mCloneScn(ms);
      if (!newscn) {
        return NULL;
      }

      /* Find first and last LOAD PHDRs. */
      CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
        if (mp->phdr.p_type != PT_LOAD) {
          continue;
        }

        if (!first || mp->phdr.p_vaddr < first->phdr.p_vaddr) {
          first = mp;
        }
        if (!last || mp->phdr.p_vaddr > last->phdr.p_vaddr) {
          /* No need to check p_memsz as segments may not overlap in memory. */
          last = mp;
        }
      }

      if (searchForCode) {
        if ((first->phdr.p_flags & PF_X) && !(first->phdr.p_flags & PF_W)) {
          injAnchor = first;
        } else if ((last->phdr.p_flags & PF_X) && !(last->phdr.p_flags & PF_W)) {
          injAnchor = last;
        } else {
          injAnchor = NULL;
        }
      } else {
        if ((first->phdr.p_flags & PF_W) && !(first->phdr.p_flags & PF_X)) {
          injAnchor = first;
        } else if ((last->phdr.p_flags & PF_W) && !(last->phdr.p_flags & PF_X)) {
          injAnchor = last;
        } else {
          injAnchor = NULL;
        }
      }

      if (!injAnchor) {
        ELFU_WARN("insertSection: Could not find injection anchor.\n"
                  "               It has to be the first or last segment in the memory image.\n");
      } else {
        GElf_Off injOffset;

        /* If the anchor is first or last, insert before or after */
        if (injAnchor == first) {
          /* Find first section and inject before it */
          ElfuScn *firstScn = elfu_mScnFirstInSegment(me, injAnchor);
          if (!firstScn) {
            ELFU_WARN("insertSection: mScnFirstInSegment failed.\n");

            // TODO: Error handling
          } else {
            injOffset = firstScn->shdr.sh_offset;

            ELFU_INFO("Inserting %s at offset 0x%jx...\n",
                      elfu_mScnName(mrel, ms),
                      injOffset);

            /* Make space */
            elfu_mInsertSpaceBefore(me, injOffset, ms->shdr.sh_size);

            /* Update memory offset */
            newscn->shdr.sh_addr = injAnchor->phdr.p_vaddr;

            /* Insert into chain of sections */
            elfu_mInsertScnInChainBefore(me, firstScn, newscn);
          }
        } else {
          /* Find last section and inject after it */
          ElfuScn *lastScn = elfu_mScnLastInSegment(me, injAnchor);
          if (!lastScn) {
            ELFU_WARN("insertSection: mScnLastInSegment failed.\n");

            // TODO: Error handling
          } else {
            injOffset = lastScn->shdr.sh_offset + SCNFILESIZE(&lastScn->shdr);

            ELFU_INFO("Expanding at offset 0x%jx...\n",
                      injOffset);

            /* Expand NOBITS sections at injection site, if any. */
            elfu_mExpandNobits(me, injOffset);

            /* Recalculate injOffset in case we expanded a NOBITS section */
            lastScn = elfu_mScnLastInSegment(me, injAnchor);
            injOffset = lastScn->shdr.sh_offset + SCNFILESIZE(&lastScn->shdr);

            ELFU_INFO("Inserting %s at offset 0x%jx...\n",
                      elfu_mScnName(mrel, ms),
                      injOffset);

            /* Make space */
            elfu_mInsertSpaceAfter(me, injOffset, ms->shdr.sh_size);

            /* Update memory offset */
            newscn->shdr.sh_addr = injAnchor->phdr.p_vaddr + (injOffset - injAnchor->phdr.p_offset);

            /* Insert into chain of sections */
            elfu_mInsertScnInChainAfter(me, lastScn, newscn);
          }
        }

        /* Update file offset in new section BEFORE we do anything else */
        newscn->shdr.sh_offset = injOffset;

        /* Inject name */
        // TODO
        newscn->shdr.sh_name = 0;

        // TODO: Relocate

        return newscn;
      }
      break;

    case DS_UNKNOWN:
      ELFU_WARN("insertSection: Don't know where to insert ' %s with flags %jd (type %d).\n",
                elfu_mScnName(mrel, ms),
                ms->shdr.sh_flags,
                ms->shdr.sh_type);
    default:
      ELFU_WARN("insertSection: Skipping section %s with flags %jd (type %d).\n",
                elfu_mScnName(mrel, ms),
                ms->shdr.sh_flags,
                ms->shdr.sh_type);
      return NULL;
  }

  if (newscn) {
    // TODO: Destroy newscn
  }
  return NULL;
}



void elfu_mReladd(ElfuElf *me, ElfuElf *mrel)
{
  ElfuScn *ms;

  assert(me);
  assert(mrel);


  /* For each section in object file, guess how to insert it */
  CIRCLEQ_FOREACH(ms, &mrel->scnList, elem) {
    ElfuScn *newscn;

    switch(ms->shdr.sh_type) {
      case SHT_NULL: /* 0 */
        continue;

      case SHT_PROGBITS: /* 1 */
        /* Find a place where it belongs and shove it in. */
        newscn = insertSection(me, mrel, ms);
        if (!newscn) {
          ELFU_WARN("mReladd: Could not insert section %s (type %d), skipping.\n",
                    elfu_mScnName(mrel, ms),
                    ms->shdr.sh_type);
        }
        break;

      case SHT_SYMTAB: /* 2 */
      case SHT_DYNSYM: /* 11 */
        /* Merge with the existing table. Take care of string tables also. */

      case SHT_STRTAB: /* 3 */
        /* May have to be merged with the existing string table for
         * the symbol table. */

      case SHT_RELA: /* 4 */
      case SHT_REL: /* 9 */
        /* Possibly append this in memory to the section model
         * that it describes. */

      case SHT_NOBITS: /* 8 */
        /* Expand this to SHT_PROGBITS, then insert as such. */

      case SHT_HASH: /* 5 */
      case SHT_DYNAMIC: /* 6 */
      case SHT_SHLIB: /* 10 */
      case SHT_SYMTAB_SHNDX: /* 18 */

      /* Don't care about the next ones yet. I've never seen
       * them and they can be implemented when necessary. */
      case SHT_NOTE: /* 7 */
      case SHT_INIT_ARRAY: /* 14 */
      case SHT_FINI_ARRAY: /* 15 */
      case SHT_PREINIT_ARRAY: /* 16 */
      case SHT_GROUP: /* 17 */
      case SHT_NUM: /* 19 */
      default:
        ELFU_WARN("mReladd: Skipping section %s (type %d).\n",
                  elfu_mScnName(mrel, ms),
                  ms->shdr.sh_type);
    }
  }
}
