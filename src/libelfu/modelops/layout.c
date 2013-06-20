#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>



static GElf_Word shiftStuffAtAfterOffset(ElfuElf *me,
                                         GElf_Off offset,
                                         GElf_Word size)
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
      mp->phdr.p_offset += size;

      elfu_mPhdrUpdateChildOffsets(mp);
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
GElf_Addr elfu_mLayoutGetSpaceInPhdr(ElfuElf *me, GElf_Word size,
                                     GElf_Word align, int w, int x,
                                     ElfuPhdr **injPhdr)
{
  ElfuPhdr *first = NULL;
  ElfuPhdr *last = NULL;
  ElfuPhdr *mp;

  assert(!(w && x));

  /* Treat read-only data as executable.
   * That's what the GNU toolchain does on x86. */
  if (!w && !x) {
    x = 1;
  }

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

      ELFU_INFO("Expanding NOBITS at address 0x%x...\n", (unsigned)endAddr);

      CIRCLEQ_FOREACH(ms, &last->childScnList, elemChildScn) {
        if (ms->shdr.sh_offset == endOff) {
          assert(ms->shdr.sh_type == SHT_NOBITS);
          assert(ms->shdr.sh_size == nobitsize);
          ms->data.d_buf = malloc(ms->shdr.sh_size);
          memset(ms->data.d_buf, '\0', ms->shdr.sh_size);
          if (!ms->data.d_buf) {
            ELFU_WARN("mExpandNobits: Could not allocate %u bytes for NOBITS expansion. Data may be inconsistent.\n",
                      (unsigned)ms->shdr.sh_size);
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

      injSpace += shiftStuffAtAfterOffset(me, endOff, nobitsize);
      injSpace -= nobitsize;
      injOffset += nobitsize;
      last->phdr.p_filesz += nobitsize;
      assert(last->phdr.p_filesz == last->phdr.p_memsz);
    }

    /* Calculate how much space we need, taking alignment into account */
    size += ROUNDUP(injOffset, align) - injOffset;

    /* If there is not enough space left, create even more. */
    if (injSpace < size) {
      injSpace += shiftStuffAtAfterOffset(me, injOffset, size - injSpace);
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
    assert(size <= shiftStuffAtAfterOffset(me, injOffset, size));

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




static int cmpPhdrOffs(const void *mp1, const void *mp2)
{
  ElfuPhdr *p1;
  ElfuPhdr *p2;

  assert(mp1);
  assert(mp2);

  p1 = *(ElfuPhdr**)mp1;
  p2 = *(ElfuPhdr**)mp2;

  assert(p1);
  assert(p2);


  if (p1->phdr.p_offset < p2->phdr.p_offset) {
    return -1;
  } else if (p1->phdr.p_offset == p2->phdr.p_offset) {
    return 0;
  } else /* if (p1->phdr.p_offset > p2->phdr.p_offset) */ {
    return 1;
  }
}

int elfu_mLayoutAuto(ElfuElf *me)
{
  ElfuPhdr *mp;
  ElfuScn *ms;
  ElfuPhdr **phdrArr;
  GElf_Off lastend = 0;
  size_t i, j;

  assert(me);

  phdrArr = malloc(elfu_mPhdrCount(me) * sizeof(*phdrArr));
  if (!phdrArr) {
    ELFU_WARN("elfu_mLayoutAuto: malloc failed for phdrArr.\n");
    return 1;
  }

  i = 0;
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }

    phdrArr[i] = mp;
    i++;
  }

  /* Assume we have at least one LOAD PHDR,
   * and that it ends after EHDR and PHDRs */
  assert(i > 1);

  /* Sort array by file offset */
  qsort(phdrArr, i, sizeof(*phdrArr), cmpPhdrOffs);

  lastend = OFFS_END(phdrArr[0]->phdr.p_offset, phdrArr[0]->phdr.p_filesz);

  /* Wiggle offsets of 2nd, 3rd etc so take minimum space */
  for (j = 1; j < i; j++) {
    GElf_Off subalign = phdrArr[j]->phdr.p_offset % phdrArr[j]->phdr.p_align;

    if ((lastend % phdrArr[j]->phdr.p_align) <= subalign) {
      lastend += subalign - (lastend % phdrArr[j]->phdr.p_align);
    } else {
      lastend += phdrArr[j]->phdr.p_align - ((lastend % phdrArr[j]->phdr.p_align) - subalign);
    }

    phdrArr[j]->phdr.p_offset = lastend;

    elfu_mPhdrUpdateChildOffsets(phdrArr[j]);

    lastend = OFFS_END(phdrArr[j]->phdr.p_offset, phdrArr[j]->phdr.p_filesz);
  }

  free(phdrArr);


  /* Place orphaned sections afterwards, maintaining alignment */
  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elemChildScn) {
    lastend = ROUNDUP(lastend, ms->shdr.sh_addralign);

    ms->shdr.sh_offset = lastend;

    lastend = OFFS_END(ms->shdr.sh_offset, SCNFILESIZE(&ms->shdr));
  }


  /* Move SHDRs to end */
  lastend = ROUNDUP(lastend, 8);
  me->ehdr.e_shoff = lastend;


  return 0;
}
