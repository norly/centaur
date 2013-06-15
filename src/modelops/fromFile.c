#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>


static void parseSymtab32(ElfuScn *ms, ElfuScn**origScnArr)
{
  size_t i;

  assert(ms);
  assert(ms->data.d_buf);
  assert(origScnArr);


  for (i = 1; (i + 1) * sizeof(Elf32_Sym) <= ms->shdr.sh_size; i++) {
    Elf32_Sym *cursym = &(((Elf32_Sym*)ms->data.d_buf)[i]);
    ElfuSym *sym = malloc(sizeof(*sym));
    assert(sym);

    sym->name = cursym->st_name;
    sym->value = cursym->st_value;
    sym->size = cursym->st_size;
    sym->bind = ELF32_ST_BIND(cursym->st_info);
    sym->type = ELF32_ST_TYPE(cursym->st_info);
    sym->other = cursym->st_other;
    sym->shndx = cursym->st_shndx;

    switch (cursym->st_shndx) {
      case SHN_UNDEF:
      case SHN_ABS:
      case SHN_COMMON:
        sym->scnptr = NULL;
        break;
      default:
        sym->scnptr = origScnArr[cursym->st_shndx - 1];
        break;
    }


    CIRCLEQ_INSERT_TAIL(&ms->symtab.syms, sym, elem);
  }
}


static void parseReltab32(ElfuScn *ms)
{
  size_t i;

  assert(ms);
  assert(ms->data.d_buf);


  for (i = 0; (i + 1) * sizeof(Elf32_Rel) <= ms->shdr.sh_size; i++) {
    Elf32_Rel *currel = &(((Elf32_Rel*)ms->data.d_buf)[i]);
    ElfuRel *rel;

    rel = malloc(sizeof(*rel));
    assert(rel);

    rel->offset = currel->r_offset;

    rel->sym = ELF32_R_SYM(currel->r_info);
    rel->type = ELF32_R_TYPE(currel->r_info);

    rel->addendUsed = 0;
    rel->addend = 0;

    CIRCLEQ_INSERT_TAIL(&ms->reltab.rels, rel, elem);
  }
}


static int cmpScnOffs(const void *ms1, const void *ms2)
{
  assert(ms1);
  assert(ms2);

  ElfuScn *s1 = *(ElfuScn**)ms1;
  ElfuScn *s2 = *(ElfuScn**)ms2;

  assert(s1);
  assert(s2);


  if (s1->shdr.sh_offset < s2->shdr.sh_offset) {
    return -1;
  } else if (s1->shdr.sh_offset == s2->shdr.sh_offset) {
    return 0;
  } else /* if (s1->shdr.sh_offset > s2->shdr.sh_offset) */ {
    return 1;
  }
}



static ElfuPhdr* parentPhdr(ElfuElf *me, ElfuScn *ms)
{
  ElfuPhdr *mp;

  assert(me);
  assert(ms);

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }

    if (PHDR_CONTAINS_SCN_IN_MEMORY(&mp->phdr, &ms->shdr)) {
      return mp;
    }

    /* Give sections a second chance if they do not have any sh_addr
     * at all. */
    /* Actually we don't, because it's ambiguous.
     * Re-enable for experiments with strangely-formatted files.
    if (ms->shdr.sh_addr == 0
        && PHDR_CONTAINS_SCN_IN_FILE(&mp->phdr, &ms->shdr)
        && OFFS_END(ms->shdr.sh_offset, ms->shdr.sh_size)
            <= OFFS_END(mp->phdr.p_offset, mp->phdr.p_memsz)) {
      return mp;
    }
    */
  }

  return NULL;
}


static ElfuPhdr* modelFromPhdr(GElf_Phdr *phdr)
{
  ElfuPhdr *mp;

  assert(phdr);

  mp = malloc(sizeof(ElfuPhdr));
  if (!mp) {
    ELFU_WARN("modelFromPhdr: malloc() failed for ElfuPhdr.\n");
    return NULL;
  }

  mp->phdr = *phdr;

  CIRCLEQ_INIT(&mp->childScnList);
  CIRCLEQ_INIT(&mp->childPhdrList);

  return mp;
}


static ElfuScn* modelFromSection(Elf_Scn *scn)
{
  ElfuScn *ms;

  assert(scn);

  ms = malloc(sizeof(ElfuScn));
  if (!ms) {
    ELFU_WARN("modelFromSection: malloc() failed for ElfuScn.\n");
    goto ERROR;
  }


  assert(gelf_getshdr(scn, &ms->shdr) == &ms->shdr);


  /* Copy each data part in source segment */
  ms->data.d_align = 1;
  ms->data.d_buf  = NULL;
  ms->data.d_off  = 0;
  ms->data.d_type = ELF_T_BYTE;
  ms->data.d_size = ms->shdr.sh_size;
  ms->data.d_version = elf_version(EV_NONE);
  if (ms->shdr.sh_type != SHT_NOBITS
      && ms->shdr.sh_size > 0) {
    Elf_Data *data;

    ms->data.d_buf = malloc(ms->shdr.sh_size);
    if (!ms->data.d_buf) {
      ELFU_WARN("modelFromSection: malloc() failed for data buffer (%jx bytes).\n", ms->shdr.sh_size);
      goto ERROR;
    }

    /* A non-empty section should contain at least one data block. */
    data = elf_rawdata(scn, NULL);
    assert(data);

    ms->data.d_align = data->d_align;
    ms->data.d_type = data->d_type;
    ms->data.d_version = data->d_version;

    while (data) {
      if (data->d_off + data->d_size > ms->shdr.sh_size) {
        ELFU_WARN("modelFromSection: libelf delivered a bogus data blob. Skipping\n");
      } else {
        memcpy(ms->data.d_buf + data->d_off, data->d_buf, data->d_size);
      }

      data = elf_rawdata(scn, data);
    }
  }

  ms->linkptr = NULL;
  ms->infoptr = NULL;

  ms->oldptr = NULL;

  CIRCLEQ_INIT(&ms->symtab.syms);
  CIRCLEQ_INIT(&ms->reltab.rels);


  return ms;

  ERROR:
  if (ms) {
    free(ms);
  }
  return NULL;
}




ElfuElf* elfu_mFromElf(Elf *e)
{
  ElfuElf *me;
  size_t shstrndx;
  size_t i, numPhdr, numShdr;
  ElfuScn **secArray = NULL;

  assert(e);
  if (elfu_eCheck(e)) {
    goto ERROR;
  }

  me = malloc(sizeof(ElfuElf));
  if (!me) {
    ELFU_WARN("elfu_mFromElf: malloc() failed for ElfuElf.\n");
    goto ERROR;
  }


  /* General stuff */
  CIRCLEQ_INIT(&me->phdrList);
  CIRCLEQ_INIT(&me->orphanScnList);
  me->shstrtab = NULL;
  me->symtab = NULL;

  me->elfclass = gelf_getclass(e);
  assert(me->elfclass != ELFCLASSNONE);
  assert(gelf_getehdr(e, &me->ehdr) == &me->ehdr);


  /* Get the section string table index */
  if (elf_getshdrstrndx(e, &shstrndx) != 0) {
    shstrndx = 0;
  }


  /* Load segments */
  assert(!elf_getphdrnum(e, &numPhdr));
  for (i = 0; i < numPhdr; i++) {
    GElf_Phdr phdr;
    ElfuPhdr *mp;

    assert(gelf_getphdr(e, i, &phdr) == &phdr);

    mp = modelFromPhdr(&phdr);
    if (!mp) {
      goto ERROR;
    }

    CIRCLEQ_INSERT_TAIL(&me->phdrList, mp, elem);
  }

  if (numPhdr > 0) {
    ElfuPhdr *mp;

    /* Find PHDR -> PHDR dependencies (needs sorted sections) */
    CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
      ElfuPhdr *mp2;

      if (mp->phdr.p_type != PT_LOAD) {
        continue;
      }

      CIRCLEQ_FOREACH(mp2, &me->phdrList, elem) {
        if (mp2 == mp) {
          continue;
        }

        if (mp->phdr.p_vaddr <= mp2->phdr.p_vaddr
            && OFFS_END(mp2->phdr.p_vaddr, mp2->phdr.p_memsz) <= OFFS_END(mp->phdr.p_vaddr, mp->phdr.p_memsz)) {
          CIRCLEQ_INSERT_TAIL(&mp->childPhdrList, mp2, elemChildPhdr);
        }
      }
    }
  }


  /* Load sections */
  assert(!elf_getshdrnum(e, &numShdr));
  if (numShdr > 1) {
    secArray = malloc((numShdr - 1) * sizeof(*secArray));
    if (!secArray) {
      ELFU_WARN("elfu_mFromElf: malloc() failed for secArray.\n");
      goto ERROR;
    }

    for (i = 1; i < numShdr; i++) {
      Elf_Scn *scn;
      ElfuScn *ms;

      scn = elf_getscn(e, i);
      assert(scn);

      ms = modelFromSection(scn);
      if (!ms) {
        goto ERROR;
      }

      secArray[i-1] =  ms;

      if (i == shstrndx) {
        me->shstrtab = ms;
      }
    }


    /* Find sh_link and sh_info dependencies (needs sections in original order) */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      switch (ms->shdr.sh_type) {
        case SHT_REL:
        case SHT_RELA:
          if (ms->shdr.sh_info > 0) {
            ms->infoptr = secArray[ms->shdr.sh_info - 1];
          }
        case SHT_DYNAMIC:
        case SHT_HASH:
        case SHT_SYMTAB:
        case SHT_DYNSYM:
        case SHT_GNU_versym:
        case SHT_GNU_verdef:
        case SHT_GNU_verneed:
          if (ms->shdr.sh_link > 0) {
            ms->linkptr = secArray[ms->shdr.sh_link - 1];
          }
      }
    }


    /* Parse symtabs (needs sections in original order) */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      switch (ms->shdr.sh_type) {
        case SHT_SYMTAB:
          me->symtab = ms;
        case SHT_DYNSYM:
          if (me->elfclass == ELFCLASS32) {
            parseSymtab32(ms, secArray);
          } else if (me->elfclass == ELFCLASS64) {
            // TODO
          }
          break;
      }
    }


    /* Parse relocations */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      switch (ms->shdr.sh_type) {
        case SHT_REL:
          if (me->elfclass == ELFCLASS32) {
            parseReltab32(ms);
          } else if (me->elfclass == ELFCLASS64) {
            // TODO
          }
          break;
        case SHT_RELA:
          if (me->elfclass == ELFCLASS32) {
            // TODO
          } else if (me->elfclass == ELFCLASS64) {
            // TODO
          }
          break;
      }
    }


    /* Sort sections by file offset */
    qsort(secArray, numShdr - 1, sizeof(*secArray), cmpScnOffs);


    /* Find PHDR -> Section dependencies (needs sorted sections) */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      ElfuPhdr *parent = parentPhdr(me, ms);

      if (parent) {
        GElf_Off shaddr = parent->phdr.p_vaddr +
                         (ms->shdr.sh_offset - parent->phdr.p_offset);

        if (ms->shdr.sh_addr == 0) {
          ms->shdr.sh_addr = shaddr;
        } else {
          assert(ms->shdr.sh_addr == shaddr);
        }

        CIRCLEQ_INSERT_TAIL(&parent->childScnList, ms, elemChildScn);
      } else {
        CIRCLEQ_INSERT_TAIL(&me->orphanScnList, ms, elemChildScn);
      }
    }
  }


  return me;


  ERROR:
  if (secArray) {
    free(secArray);
  }
  if (me) {
    // TODO: Free data structures
  }

  ELFU_WARN("elfu_mFromElf: Failed to load file.\n");
  return NULL;
}
