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





static ElfuPhdr* appendPhdr(ElfuElf *me)
{
  ElfuPhdr *phdrmp;
  ElfuPhdr *newmp;

  ELFU_DEBUG("Appending new PHDR\n");

  /* See if we have enough space for more PHDRs. If not, expand
   * the PHDR they are in. */
  phdrmp = elfu_mPhdrByOffset(me, me->ehdr.e_phoff);
  if (!phdrmp) {
    /* No LOAD maps PHDRs into memory. Let re-layouter move them. */
  } else {
    GElf_Off phdr_maxsz = OFFS_END(phdrmp->phdr.p_offset, phdrmp->phdr.p_filesz);
    ElfuScn *firstms = CIRCLEQ_FIRST(&phdrmp->childScnList);

    /* How much can the PHDR table expand within its LOAD segment? */
    if (firstms) {
      phdr_maxsz = MIN(firstms->shdr.sh_offset, phdr_maxsz);
    }
    phdr_maxsz -= me->ehdr.e_phoff;

    /* If we don't have enough space, try to make some by expanding
     * the LOAD segment we are in. There is no other way. */
    if (phdr_maxsz < (me->ehdr.e_phnum + 1) * me->ehdr.e_phentsize) {
      ElfuPhdr *mp;
      ElfuScn *ms;
      GElf_Word size = ROUNDUP(me->ehdr.e_phentsize, phdrmp->phdr.p_align);

      /* Move our sections */
      CIRCLEQ_FOREACH(ms, &phdrmp->childScnList, elemChildScn) {
        if (ms->shdr.sh_offset >= me->ehdr.e_phoff) {
          ms->shdr.sh_offset += size;
        }
      }

      /* Move our PHDRs */
      CIRCLEQ_FOREACH(mp, &phdrmp->childPhdrList, elemChildPhdr) {
        if (mp->phdr.p_offset > me->ehdr.e_phoff) {
          mp->phdr.p_offset += size;
        } else {
          mp->phdr.p_vaddr -= size;
          mp->phdr.p_paddr -= size;
        }

        if (mp->phdr.p_type == PT_PHDR) {
          mp->phdr.p_filesz += me->ehdr.e_phentsize;
          mp->phdr.p_memsz += me->ehdr.e_phentsize;
        }
      }

      /* Move other PHDRs and sections */
      assert(size <= shiftStuffAtAfterOffset(me, me->ehdr.e_phoff + 1, size));

      /* Remap ourselves */
      phdrmp->phdr.p_vaddr -= size;
      phdrmp->phdr.p_paddr -= size;
      phdrmp->phdr.p_filesz += size;
      phdrmp->phdr.p_memsz += size;
    }
  }

  newmp = elfu_mPhdrAlloc();
  assert(newmp);
  CIRCLEQ_INSERT_TAIL(&me->phdrList, newmp, elem);

  return newmp;
}





/* Finds a suitable PHDR to insert a hole into and expands it
 * if necessary.
 * Returns memory address the hole will be mapped to, or 0 if
 * the operation failed. */
GElf_Addr elfu_mLayoutGetSpaceInPhdr(ElfuElf *me, GElf_Word size,
                                     GElf_Word align, int w, int x,
                                     ElfuPhdr **injPhdr)
{
  ElfuPhdr *lowestAddr;
  ElfuPhdr *highestAddr;
  ElfuPhdr *lowestOffs;
  ElfuPhdr *highestOffsEnd;
  ElfuPhdr *mp;

  assert(!(w && x));

  /* Treat read-only data as executable.
   * That's what the GNU toolchain does on x86. */
  if (!w && !x) {
    x = 1;
  }

  /* Find first and last LOAD PHDRs. */
  elfu_mPhdrLoadLowestHighest(me, &lowestAddr, &highestAddr,
                              &lowestOffs, &highestOffsEnd);

  if (((w && (highestAddr->phdr.p_flags & PF_W))
      || (x && (highestAddr->phdr.p_flags & PF_X)))
      /* Merging only works if the LOAD is the last both in file and mem */
      && highestAddr == highestOffsEnd) {
    /* Need to append. */
    GElf_Off injOffset = OFFS_END(highestAddr->phdr.p_offset, highestAddr->phdr.p_filesz);
    GElf_Word injSpace = 0;
    GElf_Word nobitsize = highestAddr->phdr.p_memsz - highestAddr->phdr.p_filesz;

    /* Expand NOBITS if any */
    if (nobitsize > 0) {
      GElf_Off endOff = OFFS_END(highestAddr->phdr.p_offset, highestAddr->phdr.p_filesz);
      GElf_Off endAddr = OFFS_END(highestAddr->phdr.p_vaddr, highestAddr->phdr.p_filesz);
      ElfuScn *ms;

      ELFU_INFO("Expanding NOBITS at address 0x%x...\n", (unsigned)endAddr);

      CIRCLEQ_FOREACH(ms, &highestAddr->childScnList, elemChildScn) {
        if (ms->shdr.sh_offset == endOff) {
          assert(ms->shdr.sh_type == SHT_NOBITS);
          assert(ms->shdr.sh_size == nobitsize);
          ms->databuf = malloc(ms->shdr.sh_size);
          memset(ms->databuf, '\0', ms->shdr.sh_size);
          if (!ms->databuf) {
            ELFU_WARN("mExpandNobits: Could not allocate %u bytes for NOBITS expansion. Data may be inconsistent.\n",
                      (unsigned)ms->shdr.sh_size);
            assert(0);
            goto ERROR;
          }

          ms->shdr.sh_type = SHT_PROGBITS;
          ms->shdr.sh_addr = endAddr;
        }
      }

      injSpace += shiftStuffAtAfterOffset(me, endOff, nobitsize);
      injSpace -= nobitsize;
      injOffset += nobitsize;
      highestAddr->phdr.p_filesz += nobitsize;
      assert(highestAddr->phdr.p_filesz == highestAddr->phdr.p_memsz);
    }

    /* Calculate how much space we need, taking alignment into account */
    size += ROUNDUP(injOffset, align) - injOffset;

    /* If there is not enough space left, create even more. */
    if (injSpace < size) {
      injSpace += shiftStuffAtAfterOffset(me, injOffset, size - injSpace);
    }
    assert(injSpace >= size);

    /* Remap ourselves */
    highestAddr->phdr.p_filesz += size;
    highestAddr->phdr.p_memsz += size;

    injOffset = ROUNDUP(injOffset, align);

    if (injPhdr) {
      *injPhdr = highestAddr;
    }
    return highestAddr->phdr.p_vaddr + (injOffset - highestAddr->phdr.p_offset);
  } else if (((w && (lowestAddr->phdr.p_flags & PF_W))
              || (x && (lowestAddr->phdr.p_flags & PF_X)))
             && /* Enough space to expand downwards? */
             (lowestAddr->phdr.p_vaddr > 3 * lowestAddr->phdr.p_align)
             /* Merging only works if the LOAD is the first both in file and mem */
             && lowestAddr == lowestOffs) {
    /* Need to prepend or split up the PHDR. */
    GElf_Off injOffset = OFFS_END(lowestAddr->phdr.p_offset,
                                  lowestAddr->phdr.p_filesz);
    ElfuScn *ms;

    /* Round up size to take PHDR alignment into account.
     * We assume that this is a multiple of the alignment asked for. */
    assert(lowestAddr->phdr.p_align >= align);
    size = ROUNDUP(size, lowestAddr->phdr.p_align);

    /* Find first section. We assume there is at least one. */
    assert(!CIRCLEQ_EMPTY(&lowestAddr->childScnList));
    injOffset = CIRCLEQ_FIRST(&lowestAddr->childScnList)->shdr.sh_offset;

    /* Move our sections */
    CIRCLEQ_FOREACH(ms, &lowestAddr->childScnList, elemChildScn) {
      if (ms->shdr.sh_offset >= injOffset) {
        ms->shdr.sh_offset += size;
      }
    }

    /* Move our PHDRs */
    CIRCLEQ_FOREACH(mp, &lowestAddr->childPhdrList, elemChildPhdr) {
      if (mp->phdr.p_offset >= injOffset) {
        mp->phdr.p_offset += size;
      } else {
        mp->phdr.p_vaddr -= size;
        mp->phdr.p_paddr -= size;
      }
    }

    /* Move other PHDRs and sections */
    assert(size <= shiftStuffAtAfterOffset(me, injOffset + 1, size));

    /* Remap ourselves */
    lowestAddr->phdr.p_vaddr -= size;
    lowestAddr->phdr.p_paddr -= size;
    lowestAddr->phdr.p_filesz += size;
    lowestAddr->phdr.p_memsz += size;

    injOffset = ROUNDUP(injOffset, align);

    if (injPhdr) {
      *injPhdr = lowestAddr;
    }
    return lowestAddr->phdr.p_vaddr + (injOffset - lowestAddr->phdr.p_offset);
  } else {
    ElfuPhdr *newmp;
    GElf_Off injOffset;
    GElf_Addr injAddr;

    /* Add a new LOAD PHDR. */
    newmp = appendPhdr(me);

    /* ELF spec: We need (p_offset % p_align) == (p_vaddr % p_align) */
    injOffset = OFFS_END(highestOffsEnd->phdr.p_offset, highestOffsEnd->phdr.p_filesz);
    injOffset = ROUNDUP(injOffset, align);
    injAddr = OFFS_END(highestAddr->phdr.p_vaddr, highestAddr->phdr.p_memsz);
    injAddr = ROUNDUP(injAddr, highestAddr->phdr.p_align);
    injAddr += injOffset % highestAddr->phdr.p_align;

    newmp->phdr.p_align = highestAddr->phdr.p_align;
    newmp->phdr.p_filesz = size;
    newmp->phdr.p_memsz = size;
    newmp->phdr.p_flags = PF_R | (x ? PF_X : 0) | (w ? PF_W : 0);
    newmp->phdr.p_type = PT_LOAD;
    newmp->phdr.p_offset = injOffset;
    newmp->phdr.p_vaddr = injAddr;
    newmp->phdr.p_paddr = newmp->phdr.p_vaddr;

    *injPhdr = newmp;

    return injAddr;
  }




  ERROR:
  if (injPhdr) {
    *injPhdr = NULL;
  }
  return 0;
}





int elfu_mLayoutAuto(ElfuElf *me)
{
  ElfuPhdr *lowestAddr;
  ElfuPhdr *highestAddr;
  ElfuPhdr *lowestOffs;
  ElfuPhdr *highestOffsEnd;
  ElfuPhdr *mp;
  ElfuScn *ms;
  ElfuPhdr **phdrArr;
  GElf_Off lastend = 0;

  assert(me);

  /* Find first and last LOAD PHDRs. */
  elfu_mPhdrLoadLowestHighest(me, &lowestAddr, &highestAddr,
                              &lowestOffs, &highestOffsEnd);

  phdrArr = malloc(elfu_mPhdrCount(me) * sizeof(*phdrArr));
  if (!phdrArr) {
    ELFU_WARN("elfu_mLayoutAuto: malloc failed for phdrArr.\n");
    return 1;
  }


  lastend = OFFS_END(highestOffsEnd->phdr.p_offset, highestOffsEnd->phdr.p_filesz);


  /* If PHDRs are not mapped into memory, place them after LOAD segments. */
  mp = elfu_mPhdrByOffset(me, me->ehdr.e_phoff);
  if (!mp) {
    lastend = ROUNDUP(lastend, 8);
    me->ehdr.e_phoff = lastend;
    lastend += me->ehdr.e_phnum * me->ehdr.e_phentsize;
  } else {
    /* Update size of PHDR PHDR */
    ElfuPhdr *phdrmp;

    CIRCLEQ_FOREACH(phdrmp, &mp->childPhdrList, elemChildPhdr) {
      if (phdrmp->phdr.p_type == PT_PHDR) {
        phdrmp->phdr.p_filesz = elfu_mPhdrCount(me) * me->ehdr.e_phentsize;
        phdrmp->phdr.p_memsz = elfu_mPhdrCount(me) * me->ehdr.e_phentsize;
      }
    }
  }


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
