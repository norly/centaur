#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


static void flattenSymtab(ElfuElf *me)
{
  ElfuSym *sym;
  size_t numsyms = 0;

  elfu_mLayoutAuto(me);

  /* Update section indexes and count symbols */
  CIRCLEQ_FOREACH(sym, &me->symtab->symtab.syms, elem) {
    if (sym->scnptr) {
      sym->shndx = elfu_mScnIndex(me, sym->scnptr);
    }

    numsyms++;
  }

  /* Copy symbols to elfclass-specific format */
  if (me->elfclass == ELFCLASS32) {
    size_t newsize = (numsyms + 1) * sizeof(Elf32_Sym);
    size_t i;

    if (me->symtab->data.d_buf) {
      free(me->symtab->data.d_buf);
    }
    me->symtab->data.d_buf = malloc(newsize);
    assert(me->symtab->data.d_buf);

    me->symtab->data.d_size = newsize;
    me->symtab->shdr.sh_size = newsize;
    memset(me->symtab->data.d_buf, 0, newsize);

    i = 1;
    CIRCLEQ_FOREACH(sym, &me->symtab->symtab.syms, elem) {
      Elf32_Sym *es = ((Elf32_Sym*)me->symtab->data.d_buf) + i;

      es->st_name = sym->name;
      es->st_value = sym->value;
      es->st_size = sym->size;
      es->st_info = ELF32_ST_INFO(sym->bind, sym->type);
      es->st_other = sym->other;
      es->st_shndx = sym->shndx;

      i++;
    }
  } else if (me->elfclass == ELFCLASS64) {
    size_t newsize = (numsyms + 1) * sizeof(Elf64_Sym);
    size_t i;

    if (me->symtab->data.d_buf) {
      free(me->symtab->data.d_buf);
    }
    me->symtab->data.d_buf = malloc(newsize);
    assert(me->symtab->data.d_buf);

    me->symtab->data.d_size = newsize;
    me->symtab->shdr.sh_size = newsize;
    memset(me->symtab->data.d_buf, 0, newsize);

    i = 1;
    CIRCLEQ_FOREACH(sym, &me->symtab->symtab.syms, elem) {
      Elf64_Sym *es = ((Elf64_Sym*)me->symtab->data.d_buf) + i;

      es->st_name = sym->name;
      es->st_value = sym->value;
      es->st_size = sym->size;
      es->st_info = ELF64_ST_INFO(sym->bind, sym->type);
      es->st_other = sym->other;
      es->st_shndx = sym->shndx;

      i++;
    }
  } else {
    // Unknown elfclass
    assert(0);
  }

  elfu_mLayoutAuto(me);
}


static void modelToPhdrs(ElfuElf *me, Elf *e)
{
  ElfuPhdr *mp;
  size_t i;

  /* Count PHDRs */
  i = 0;
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    i++;
  }

  if (!gelf_newphdr(e, i)) {
    ELFU_WARNELF("gelf_newphdr");
  }

  /* Copy PHDRs */
  i = 0;
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (!gelf_update_phdr (e, i, &mp->phdr)) {
      ELFU_WARNELF("gelf_update_phdr");
    }

    i++;
  }
}



static void* modelToSection(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  (void) me;
  (void) aux2;
  Elf *e = (Elf*)aux1;
  Elf_Scn *scnOut;

  scnOut = elf_newscn(e);
  if (!scnOut) {
    ELFU_WARNELF("elf_newscn");
    return (void*)-1;
  }


  /* SHDR */
  if (ms->linkptr) {
    ms->shdr.sh_link = elfu_mScnIndex(me, ms->linkptr);
  }
  if (ms->infoptr) {
    ms->shdr.sh_info = elfu_mScnIndex(me, ms->infoptr);
  }
  if (!gelf_update_shdr(scnOut, &ms->shdr)) {
    ELFU_WARNELF("gelf_update_shdr");
  }


  /* Data */
  if (ms->data.d_buf) {
    Elf_Data *dataOut = elf_newdata(scnOut);
    if (!dataOut) {
      ELFU_WARNELF("elf_newdata");
    }

    dataOut->d_align = ms->data.d_align;
    dataOut->d_buf  = ms->data.d_buf;
    dataOut->d_off  = ms->data.d_off;
    dataOut->d_type = ms->data.d_type;
    dataOut->d_size = ms->data.d_size;
    dataOut->d_version = ms->data.d_version;
  }

  return NULL;
}





void elfu_mToElf(ElfuElf *me, Elf *e)
{
  if (me->symtab) {
    flattenSymtab(me);
  }


  /* We control the ELF file's layout now. */
  /* tired's libelf also offers ELF_F_LAYOUT_OVERLAP for overlapping sections,
   * but we don't want that since we filtered it out in the reading stage
   * already. It would be too mind-blowing to handle the dependencies between
   * the PHDRs and sections then... */
  elf_flagelf(e, ELF_C_SET, ELF_F_LAYOUT);


  /* EHDR */
  if (!gelf_newehdr(e, me->elfclass)) {
    ELFU_WARNELF("gelf_newehdr");
  }

  if (me->shstrtab) {
    me->ehdr.e_shstrndx = elfu_mScnIndex(me, me->shstrtab);
  }

  if (!gelf_update_ehdr(e, &me->ehdr)) {
    ELFU_WARNELF("gelf_update_ehdr");
  }


  /* Sections */
  elfu_mScnForall(me, modelToSection, e, NULL);


  /* PHDRs */
  modelToPhdrs(me, e);


  elf_flagelf(e, ELF_C_SET, ELF_F_DIRTY);
}
