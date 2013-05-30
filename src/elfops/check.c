#include <assert.h>
#include <stdlib.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>

int elfu_eCheck(Elf *e)
{
  int elfclass;
  GElf_Ehdr ehdr;
  GElf_Phdr *phdrs = NULL;
  GElf_Shdr *shdrs = NULL;
  size_t i, j, numPhdr, numShdr;
  int retval = 0;

  assert(e);

  elfclass = gelf_getclass(e);
  if (elfclass == ELFCLASSNONE) {
    ELFU_WARNELF("getclass");
    goto ERROR;
  }

  if (!gelf_getehdr(e, &ehdr)) {
    ELFU_WARNELF("gelf_getehdr");
    goto ERROR;
  }

  if (elf_getphdrnum(e, &numPhdr)) {
    ELFU_WARNELF("elf_getphdrnum");
    goto ERROR;
  }

  if (elf_getshdrnum(e, &numShdr)) {
    ELFU_WARNELF("elf_getshdrnum");
    goto ERROR;
  }


  if (numPhdr > 0) {
    phdrs = malloc(numPhdr * sizeof(GElf_Phdr));
    if (!phdrs) {
      ELFU_WARN("elfu_eCheck: malloc() failed for phdrs.\n");
      goto ERROR;
    }

    /* Attempt to load all PHDRs at once to catch any errors early */
    for (i = 0; i < numPhdr; i++) {
      GElf_Phdr phdr;
      if (gelf_getphdr(e, i, &phdr) != &phdr) {
        ELFU_WARN("gelf_getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
        goto ERROR;
      }

      phdrs[i] = phdr;
    }

    /* Check that LOAD PHDR memory ranges do not overlap, and that others
     * are either fully contained in a LOAD range, or not at all. */
    for (i = 0; i < numPhdr; i++) {
      if (phdrs[i].p_type != PT_LOAD) {
        continue;
      }

      for (j = 0; j < numPhdr; j++) {
        if (j == i || phdrs[j].p_type != PT_LOAD) {
          continue;
        }

        if (OVERLAPPING(phdrs[i].p_vaddr, phdrs[i].p_memsz,
                        phdrs[j].p_vaddr, phdrs[j].p_memsz)) {
          if (phdrs[j].p_type == PT_LOAD) {
            ELFU_WARN("elfu_eCheck: Found LOAD PHDRs that overlap in memory.\n");
            goto ERROR;
          } else if (!FULLY_OVERLAPPING(phdrs[i].p_vaddr, phdrs[i].p_memsz,
                                        phdrs[j].p_vaddr, phdrs[j].p_memsz)) {
            ELFU_WARN("elfu_eCheck: PHDRs %d and %d partially overlap in memory.\n", i, j);
            goto ERROR;
          }
        }
      }
    }
  }


  if (numShdr > 1) {
    /* SHDRs should not overlap with PHDRs. */
    if (OVERLAPPING(ehdr.e_shoff, numShdr * ehdr.e_shentsize,
                    ehdr.e_phoff, numPhdr * ehdr.e_phentsize)) {
      ELFU_WARN("elfu_eCheck: SHDRs overlap with PHDRs.\n");
      goto ERROR;
    }

    shdrs = malloc(numShdr * sizeof(GElf_Shdr));
    if (!shdrs) {
      ELFU_WARN("elfu_eCheck: malloc() failed for shdrs.\n");
      goto ERROR;
    }

    /* Attempt to load all SHDRs at once to catch any errors early */
    for (i = 1; i < numShdr; i++) {
      Elf_Scn *scn;
      GElf_Shdr shdr;

      scn = elf_getscn(e, i);
      if (!scn) {
        ELFU_WARN("elf_getscn() failed for #%d: %s\n", i, elf_errmsg(-1));
      }

      if (gelf_getshdr(scn, &shdr) != &shdr) {
        ELFU_WARNELF("gelf_getshdr");
        goto ERROR;
      }

      shdrs[i] = shdr;
    }


    /* Check that Section memory ranges do not overlap.
     * NB: Section 0 is reserved and thus ignored. */
    for (i = 1; i < numShdr; i++) {
      /* Section should not overlap with EHDR. */
      if (shdrs[i].sh_offset == 0) {
        ELFU_WARN("elfu_eCheck: Section %d overlaps with EHDR.\n", i);
        goto ERROR;
      }

      /* Section should not overlap with PHDRs. */
      if (OVERLAPPING(shdrs[i].sh_offset, SCNFILESIZE(&shdrs[i]),
                      ehdr.e_phoff, numPhdr * ehdr.e_phentsize)) {
        ELFU_WARN("elfu_eCheck: Section %d overlaps with PHDR.\n", i);
        goto ERROR;
      }

      /* Section should not overlap with SHDRs. */
      if (OVERLAPPING(shdrs[i].sh_offset, SCNFILESIZE(&shdrs[i]),
                      ehdr.e_shoff, numShdr * ehdr.e_shentsize)) {
        ELFU_WARN("elfu_eCheck: Section %d overlaps with SHDRs.\n", i);
        goto ERROR;
      }

      for (j = 1; j < numShdr; j++) {
        if (j == i) {
          continue;
        }

        /* Sections must not overlap in memory. */
        if (shdrs[i].sh_addr != 0
            && shdrs[j].sh_addr != 0
            && OVERLAPPING(shdrs[i].sh_addr, shdrs[i].sh_size,
                           shdrs[j].sh_addr, shdrs[j].sh_size)) {
          ELFU_WARN("elfu_eCheck: Sections %d and %d overlap in memory.\n", i, j);
          goto ERROR;
        }

        /* Sections must not overlap in file. */
        if (OVERLAPPING(shdrs[i].sh_offset, SCNFILESIZE(&shdrs[i]),
                        shdrs[j].sh_offset, SCNFILESIZE(&shdrs[j]))) {
          ELFU_WARN("elfu_eCheck: Sections %d and %d overlap in file.\n", i, j);
          goto ERROR;
        }
      }

      /* Section addr/offset should match parent PHDR.
       * Find parent PHDR: */
      for (j = 0; j < numPhdr; j++) {
        if (PHDR_CONTAINS_SCN_IN_MEMORY(&phdrs[j], &shdrs[i])) {
          GElf_Off shoff = phdrs[j].p_offset + (shdrs[i].sh_addr - phdrs[j].p_vaddr);

          if (shdrs[i].sh_offset != shoff
              || !PHDR_CONTAINS_SCN_IN_FILE(&phdrs[j], &shdrs[i])) {
            ELFU_WARN("elfu_eCheck: Memory/file offsets/sizes are not congruent for SHDR %d, PHDR %d.\n", i, j);
            goto ERROR;
          }
        }
      }

      /* sh_link members should not point to sections out of range. */
      if (shdrs[i].sh_link >= numShdr) {
        ELFU_WARN("elfu_eCheck: Bogus sh_link in SHDR %d.\n", i);
      }
    }
  }


  DONE:
  if (phdrs) {
    free(phdrs);
  }
  if (shdrs) {
    free(shdrs);
  }
  return retval;

  ERROR:
  ELFU_WARN("elfu_eCheck: Errors found.\n");
  retval = -1;
  goto DONE;
}
