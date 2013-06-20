#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>


static void parseSymtab(ElfuElf *me, ElfuScn *ms, ElfuScn**origScnArr)
{
  ElfuSym *sym;
  size_t i;

  assert(ms);
  assert(ms->databuf);
  assert(origScnArr);

  /* Parse symbols from their elfclass-specific format */
  if (me->elfclass == ELFCLASS32) {
    for (i = 1; (i + 1) * sizeof(Elf32_Sym) <= ms->shdr.sh_size; i++) {
      Elf32_Sym *cursym = ((Elf32_Sym*)ms->databuf) + i;
      ElfuSym *newsym = malloc(sizeof(*sym));
      assert(newsym);

      newsym->name = cursym->st_name;
      newsym->value = cursym->st_value;
      newsym->size = cursym->st_size;
      newsym->bind = ELF32_ST_BIND(cursym->st_info);
      newsym->type = ELF32_ST_TYPE(cursym->st_info);
      newsym->other = cursym->st_other;
      newsym->shndx = cursym->st_shndx;

      CIRCLEQ_INSERT_TAIL(&ms->symtab.syms, newsym, elem);
    }
  } else if (me->elfclass == ELFCLASS64) {
    for (i = 1; (i + 1) * sizeof(Elf64_Sym) <= ms->shdr.sh_size; i++) {
      Elf64_Sym *cursym = ((Elf64_Sym*)ms->databuf) + i;
      ElfuSym *newsym = malloc(sizeof(*sym));
      assert(newsym);

      newsym->name = cursym->st_name;
      newsym->value = cursym->st_value;
      newsym->size = cursym->st_size;
      newsym->bind = ELF64_ST_BIND(cursym->st_info);
      newsym->type = ELF64_ST_TYPE(cursym->st_info);
      newsym->other = cursym->st_other;
      newsym->shndx = cursym->st_shndx;

      CIRCLEQ_INSERT_TAIL(&ms->symtab.syms, newsym, elem);
    }
  } else {
    /* Unknown elfclass */
    assert(0);
  }

  /* For each section, find the section it points to if any. */
  CIRCLEQ_FOREACH(sym, &ms->symtab.syms, elem) {
    switch (sym->shndx) {
      case SHN_UNDEF:
      case SHN_ABS:
      case SHN_COMMON:
        sym->scnptr = NULL;
        break;
      default:
        sym->scnptr = origScnArr[sym->shndx - 1];
        break;
    }
  }
}


static void parseReltab(ElfuElf *me, ElfuScn *ms)
{
  size_t i;

  assert(ms);
  assert(ms->databuf);

  if (me->elfclass == ELFCLASS32) {
    for (i = 0; (i + 1) * sizeof(Elf32_Rel) <= ms->shdr.sh_size; i++) {
      Elf32_Rel *currel = &(((Elf32_Rel*)ms->databuf)[i]);
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
  } else if (me->elfclass == ELFCLASS64) {
    for (i = 0; (i + 1) * sizeof(Elf64_Rel) <= ms->shdr.sh_size; i++) {
      Elf64_Rel *currel = &(((Elf64_Rel*)ms->databuf)[i]);
      ElfuRel *rel;

      rel = malloc(sizeof(*rel));
      assert(rel);

      rel->offset = currel->r_offset;
      rel->sym = ELF64_R_SYM(currel->r_info);
      rel->type = ELF64_R_TYPE(currel->r_info);
      rel->addendUsed = 0;
      rel->addend = 0;

      CIRCLEQ_INSERT_TAIL(&ms->reltab.rels, rel, elem);
    }
  } else {
    /* Unknown elfclass */
    assert(0);
  }
}


static void parseRelatab(ElfuElf *me, ElfuScn *ms)
{
  size_t i;

  assert(ms);
  assert(ms->databuf);

  if (me->elfclass == ELFCLASS32) {
    for (i = 0; (i + 1) * sizeof(Elf32_Rela) <= ms->shdr.sh_size; i++) {
      Elf32_Rela *currel = &(((Elf32_Rela*)ms->databuf)[i]);
      ElfuRel *rel;

      rel = malloc(sizeof(*rel));
      assert(rel);

      rel->offset = currel->r_offset;
      rel->sym = ELF32_R_SYM(currel->r_info);
      rel->type = ELF32_R_TYPE(currel->r_info);
      rel->addendUsed = 1;
      rel->addend = currel->r_addend;

      CIRCLEQ_INSERT_TAIL(&ms->reltab.rels, rel, elem);
    }
  } else if (me->elfclass == ELFCLASS64) {
    for (i = 0; (i + 1) * sizeof(Elf64_Rela) <= ms->shdr.sh_size; i++) {
      Elf64_Rela *currel = &(((Elf64_Rela*)ms->databuf)[i]);
      ElfuRel *rel;

      rel = malloc(sizeof(*rel));
      assert(rel);

      rel->offset = currel->r_offset;
      rel->sym = ELF64_R_SYM(currel->r_info);
      rel->type = ELF64_R_TYPE(currel->r_info);
      rel->addendUsed = 1;
      rel->addend = currel->r_addend;

      CIRCLEQ_INSERT_TAIL(&ms->reltab.rels, rel, elem);
    }
  } else {
    /* Unknown elfclass */
    assert(0);
  }
}


static int cmpScnOffs(const void *ms1, const void *ms2)
{
  ElfuScn *s1 = *(ElfuScn**)ms1;
  ElfuScn *s2 = *(ElfuScn**)ms2;

  assert(ms1);
  assert(ms2);

  s1 = *(ElfuScn**)ms1;
  s2 = *(ElfuScn**)ms2;

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
  }

  return NULL;
}


static ElfuPhdr* modelFromPhdr(GElf_Phdr *phdr)
{
  ElfuPhdr *mp;

  assert(phdr);

  mp = elfu_mPhdrAlloc();
  if (!mp) {
    return NULL;
  }

  mp->phdr = *phdr;

  return mp;
}


static ElfuScn* modelFromSection(Elf_Scn *scn)
{
  ElfuScn *ms;

  assert(scn);

  ms = elfu_mScnAlloc();
  if (!ms) {
    goto ERROR;
  }

  /* Copy SHDR */
  assert(gelf_getshdr(scn, &ms->shdr) == &ms->shdr);

  /* Copy each data part in source segment */
  if (SCNFILESIZE(&ms->shdr) > 0) {
    Elf_Data *data;

    ms->databuf = malloc(ms->shdr.sh_size);
    if (!ms->databuf) {
      ELFU_WARN("modelFromSection: malloc() failed for data buffer (%x bytes).\n", (unsigned)ms->shdr.sh_size);
      goto ERROR;
    }

    /* A non-empty section should contain at least one data block. */
    data = elf_rawdata(scn, NULL);
    assert(data);

    /* elf_rawdata() always returns ELF_T_BYTE */
    assert(data->d_type == ELF_T_BYTE);

    while (data) {
      if (data->d_off + data->d_size > ms->shdr.sh_size) {
        ELFU_WARN("modelFromSection: libelf delivered a bogus data blob. Skipping\n");
      } else {
        memcpy((char*)ms->databuf + data->d_off, data->d_buf, data->d_size);
      }

      data = elf_rawdata(scn, data);
    }
  }

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
          parseSymtab(me, ms, secArray);
          break;
      }
    }


    /* Parse relocations (needs sections in original order) */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      switch (ms->shdr.sh_type) {
        case SHT_REL:
          parseReltab(me, ms);
          break;
        case SHT_RELA:
          parseRelatab(me, ms);
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
