#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>



static GElf_Word makeSpaceAtOffset(ElfuElf *me, GElf_Off offset, GElf_Word size)
{
  ElfuPhdr *mp;
  ElfuScn *ms;
  /* Force a minimum alignment, just to be sure. */
  GElf_Word align = 64;

  /* Find maximum alignment size by which we have to shift.
   * Assumes alignment sizes are always 2^x. */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_offset >= offset) {
      if (mp->phdr.p_align > align) {
        align = mp->phdr.p_align;
      }
    }
  }

  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elemChildScn) {
    if (ms->shdr.sh_offset >= offset) {
      if (ms->shdr.sh_addralign > align) {
        align = ms->shdr.sh_addralign;
      }
    }
  }

  size = ROUNDUP(size, align);

  /* Shift stuff */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }

    if (mp->phdr.p_offset >= offset) {
      ElfuScn *ms;
      ElfuPhdr *mpc;

      mp->phdr.p_offset += size;
      CIRCLEQ_FOREACH(mpc, &mp->childPhdrList, elemChildPhdr) {
        mpc->phdr.p_offset += size;
      }
      CIRCLEQ_FOREACH(ms, &mp->childScnList, elemChildScn) {
        ms->shdr.sh_offset += size;
      }
    }
  }

  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elemChildScn) {
    if (ms->shdr.sh_offset >= offset) {
      ms->shdr.sh_offset += size;
    }
  }

  if (me->ehdr.e_phoff >= offset) {
    me->ehdr.e_phoff += size;
  }

  if (me->ehdr.e_shoff >= offset) {
    me->ehdr.e_shoff += size;
  }

  return size;
}



/* Finds a suitable PHDR to insert a hole into and expands it
 * if necessary.
 * Returns memory address the hole will be mapped to, or 0 if
 * the operation failed. */
static GElf_Addr getSpaceInPhdr(ElfuElf *me, GElf_Word size,
                                GElf_Word align, int w, int x,
                                ElfuPhdr **injPhdr)
{
  ElfuPhdr *first = NULL;
  ElfuPhdr *last = NULL;
  ElfuPhdr *mp;

  assert(!(w && x));

  /* Find first and last LOAD PHDRs.
   * Don't compare p_memsz - segments don't overlap in memory. */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }
    if (!first || mp->phdr.p_vaddr < first->phdr.p_vaddr) {
      first = mp;
    }
    if (!last || mp->phdr.p_vaddr > last->phdr.p_vaddr) {
      last = mp;
    }
  }

  if ((w && (last->phdr.p_flags & PF_W))
      || (x && (last->phdr.p_flags & PF_X))) {
    /* Need to append. */
    GElf_Off injOffset = OFFS_END(last->phdr.p_offset, last->phdr.p_filesz);
    GElf_Word injSpace = 0;
    GElf_Word nobitsize = last->phdr.p_memsz - last->phdr.p_filesz;

    /* Expand NOBITS if any */
    if (nobitsize > 0) {
      GElf_Off endOff = OFFS_END(last->phdr.p_offset, last->phdr.p_filesz);
      GElf_Off endAddr = OFFS_END(last->phdr.p_vaddr, last->phdr.p_filesz);
      ElfuScn *ms;

      ELFU_INFO("Expanding NOBITS at address 0x%jx...\n", endAddr);

      CIRCLEQ_FOREACH(ms, &last->childScnList, elemChildScn) {
        if (ms->shdr.sh_offset == endOff) {
          assert(ms->shdr.sh_type == SHT_NOBITS);
          assert(ms->shdr.sh_size == nobitsize);
          ms->data.d_buf = malloc(ms->shdr.sh_size);
          memset(ms->data.d_buf, '\0', ms->shdr.sh_size);
          if (!ms->data.d_buf) {
            ELFU_WARN("mExpandNobits: Could not allocate %jd bytes for NOBITS expansion. Data may be inconsistent.\n", ms->shdr.sh_size);
            assert(0);
            goto ERROR;
          }

          ms->data.d_align = 1;
          ms->data.d_off  = 0;
          ms->data.d_type = ELF_T_BYTE;
          ms->data.d_size = ms->shdr.sh_size;
          ms->data.d_version = elf_version(EV_NONE);

          ms->shdr.sh_type = SHT_PROGBITS;
          ms->shdr.sh_addr = endAddr;
        }
      }

      injSpace += makeSpaceAtOffset(me, endOff, nobitsize);
      injSpace -= nobitsize;
      injOffset += nobitsize;
      last->phdr.p_filesz += nobitsize;
      assert(last->phdr.p_filesz == last->phdr.p_memsz);
    }

    /* Calculate how much space we need, taking alignment into account */
    size += ROUNDUP(injOffset, align) - injOffset;

    /* If there is not enough space left, create even more. */
    if (injSpace < size) {
      injSpace += makeSpaceAtOffset(me, injOffset, size - injSpace);
    }
    assert(injSpace >= size);

    /* Remap ourselves */
    last->phdr.p_filesz += size;
    last->phdr.p_memsz += size;

    injOffset = ROUNDUP(injOffset, align);

    if (injPhdr) {
      *injPhdr = last;
    }
    return last->phdr.p_vaddr + (injOffset - last->phdr.p_offset);
  } else if ((w && (first->phdr.p_flags & PF_W))
             || (x && (first->phdr.p_flags & PF_X))) {
    /* Need to prepend or split up the PHDR. */
    GElf_Off injOffset = OFFS_END(first->phdr.p_offset, first->phdr.p_filesz);
    ElfuScn *ms;

    /* Round up size to take PHDR alignment into account.
     * We assume that this is a multiple of the alignment asked for. */
    assert(first->phdr.p_align >= align);
    size = ROUNDUP(size, first->phdr.p_align);

    /* Find first section. We assume there is at least one. */
    assert(!CIRCLEQ_EMPTY(&first->childScnList));
    injOffset = CIRCLEQ_FIRST(&first->childScnList)->shdr.sh_offset;

    /* Move our sections */
    CIRCLEQ_FOREACH(ms, &first->childScnList, elemChildScn) {
      if (ms->shdr.sh_offset >= injOffset) {
        ms->shdr.sh_offset += size;
      }
    }

    /* Move our PHDRs */
    CIRCLEQ_FOREACH(mp, &first->childPhdrList, elemChildPhdr) {
      if (mp->phdr.p_offset >= injOffset) {
        mp->phdr.p_offset += size;
      } else {
        mp->phdr.p_vaddr -= size;
        mp->phdr.p_paddr -= size;
      }
    }

    /* Move other PHDRs and sections */
    assert(size <= makeSpaceAtOffset(me, injOffset, size));

    /* Remap ourselves */
    first->phdr.p_vaddr -= size;
    first->phdr.p_paddr -= size;
    first->phdr.p_filesz += size;
    first->phdr.p_memsz += size;

    injOffset = ROUNDUP(injOffset, align);

    if (injPhdr) {
      *injPhdr = first;
    }
    return first->phdr.p_vaddr + (injOffset - first->phdr.p_offset);
  }

  ERROR:
  if (injPhdr) {
    *injPhdr = NULL;
  }
  return 0;
}




static ElfuScn* insertSection(ElfuElf *me, ElfuElf *mrel, ElfuScn *oldscn)
{
  ElfuScn *newscn = NULL;
  GElf_Addr injAddr;
  GElf_Off injOffset;
  ElfuPhdr *injPhdr;

  if (oldscn->shdr.sh_flags & SHF_ALLOC) {
    newscn = elfu_mCloneScn(oldscn);
    if (!newscn) {
      return NULL;
    }


    injAddr = getSpaceInPhdr(me, newscn->shdr.sh_size,
                             newscn->shdr.sh_addralign,
                             newscn->shdr.sh_flags & SHF_WRITE,
                             newscn->shdr.sh_flags & SHF_EXECINSTR,
                             &injPhdr);

    if (!injPhdr) {
      ELFU_WARN("insertSection: Could not find a place to insert section.\n");
      goto ERROR;
    }

    ELFU_INFO("Inserting %s at address 0x%jx...\n",
              elfu_mScnName(mrel, oldscn),
              injAddr);

    injOffset = injAddr - injPhdr->phdr.p_vaddr + injPhdr->phdr.p_offset;

    newscn->shdr.sh_addr = injAddr;
    newscn->shdr.sh_offset = injOffset;

    if (CIRCLEQ_EMPTY(&injPhdr->childScnList)
        || CIRCLEQ_LAST(&injPhdr->childScnList)->shdr.sh_offset < injOffset) {
      CIRCLEQ_INSERT_TAIL(&injPhdr->childScnList, newscn, elemChildScn);
    } else {
      ElfuScn *ms;
      CIRCLEQ_FOREACH(ms, &injPhdr->childScnList, elemChildScn) {
        if (injOffset < ms->shdr.sh_offset) {
          CIRCLEQ_INSERT_BEFORE(&injPhdr->childScnList, ms, newscn, elemChildScn);
          break;
        }
      }
    }

    /* Inject name */
    // TODO
    newscn->shdr.sh_name = 0;

    // TODO: Relocate

    return newscn;
  } else {
      ELFU_WARN("insertSection: Skipping section %s with flags %jd (type %d).\n",
                elfu_mScnName(mrel, oldscn),
                oldscn->shdr.sh_flags,
                oldscn->shdr.sh_type);
      goto ERROR;
  }

  ERROR:
  if (newscn) {
    // TODO: Destroy newscn
  }
  return NULL;
}


int subScnAdd1(ElfuElf *mrel, ElfuScn *ms, void *aux1, void *aux2)
{
  (void)aux2;
  ElfuElf *me = (ElfuElf*)aux1;

  ElfuScn *newscn;

  switch(ms->shdr.sh_type) {
    case SHT_PROGBITS: /* 1 */
      /* Find a place where it belongs and shove it in. */
      newscn = insertSection(me, mrel, ms);
      if (!newscn) {
        ELFU_WARN("mReladd: Could not insert section %s (type %d), skipping.\n",
                  elfu_mScnName(mrel, ms),
                  ms->shdr.sh_type);
      }
      break;
  }

  return 0;
}


int subScnAdd2(ElfuElf *mrel, ElfuScn *ms, void *aux1, void *aux2)
{
  (void)aux2;
  ElfuElf *me = (ElfuElf*)aux1;
  (void)me;

  switch(ms->shdr.sh_type) {
    case SHT_NULL: /* 0 */
    case SHT_PROGBITS: /* 1 */
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

  return 0;
}


void elfu_mReladd(ElfuElf *me, ElfuElf *mrel)
{
  assert(me);
  assert(mrel);

  /* For each section in object file, guess how to insert it */
  elfu_mScnForall(mrel, subScnAdd1, me, NULL);

  /* Do relocations and other stuff */
  elfu_mScnForall(mrel, subScnAdd2, me, NULL);
}
