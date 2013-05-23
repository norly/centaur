#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>


void elfu_mExpandNobits(ElfuElf *me, GElf_Off off)
{
  ElfuScn *ms;
  ElfuPhdr *mp;
  GElf_Xword expansionSize;

  assert(me);

  expansionSize = 0;

  /* Find the maximum amount we need to expand by. Check PHDRs first */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    GElf_Off off2 = mp->phdr.p_offset;
    GElf_Off end2 = mp->phdr.p_offset + mp->phdr.p_filesz;
    if (end2 == off) {
      GElf_Xword size2 = mp->phdr.p_memsz - mp->phdr.p_filesz;
      if (size2 > expansionSize) {
        expansionSize = size2;
      }
    } else if (end2 > off) {
      if (off2 < off) {
        /*
         * Found a PHDR whose file contents overlaps with the section
         * to be filled. This means that it relies on the NOBITS area
         * being actually 0 bytes, and the expansion would ruin it.
         */
        fprintf(stderr, "mExpandNobits: Found PHDR spanning expansion offset. Aborting.\n");
        return;
      }
    } else {
      // end2 < off, and the PHDR is unaffected.
      continue;
    }
  }

  /* Now check SHDRs */
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if (ms->shdr.sh_offset == off) {
      if (ms->shdr.sh_type == SHT_NOBITS) {
        if (ms->shdr.sh_size > expansionSize) {
          expansionSize = ms->shdr.sh_size;
        }
      }
    }
  }



  /* Expand! */


  /* Move all following PHDR offsets further down the file. */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    GElf_Off off2 = mp->phdr.p_offset;
    GElf_Off end2 = mp->phdr.p_offset + mp->phdr.p_filesz;

    if (off2 >= off) {
      mp->phdr.p_offset += expansionSize;
    } else {
      if (end2 == off) {
        /* This PHDR now has corresponding bytes in the file for every
         * byte in memory. */
        mp->phdr.p_filesz = mp->phdr.p_memsz;
      }
    }
  }

  /* Move the following sections */
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if (ms->shdr.sh_offset >= off) {
      if (ms->shdr.sh_offset > off
          || ms->shdr.sh_type != SHT_NOBITS) {
        ms->shdr.sh_offset += expansionSize;
      }
    }
  }

  /* Move SHDR/PHDR tables */
  if (me->ehdr.e_shoff >= off) {
    me->ehdr.e_shoff += expansionSize;
  }

  if (me->ehdr.e_phoff >= off) {
    me->ehdr.e_phoff += expansionSize;
  }


  /* Convert any NOBITS at off to PROGBITS */
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if (ms->shdr.sh_offset == off) {
      if (ms->shdr.sh_type == SHT_NOBITS) {
        ms->data.d_buf = malloc(ms->shdr.sh_size);
        memset(ms->data.d_buf, '\0', ms->shdr.sh_size);
        if (!ms->data.d_buf) {
          fprintf(stderr, "mExpandNobits: Could not allocate %jd bytes for NOBITS expansion.\n", ms->shdr.sh_size);
        }

        ms->data.d_align = 1;
        ms->data.d_off  = 0;
        ms->data.d_type = ELF_T_BYTE;
        ms->data.d_size = ms->shdr.sh_size;
        ms->data.d_version = elf_version(EV_NONE);

        ms->shdr.sh_type = SHT_PROGBITS;
      }
    }
  }
}
