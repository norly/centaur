#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


int elfu_mSymtabLookupSymToAddr(ElfuElf *me, ElfuScn *msst, ElfuSym *sym, GElf_Addr *result)
{
  char *symname = ELFU_SYMSTR(msst, sym->name);

  switch (sym->type) {
    case STT_NOTYPE:
    case STT_OBJECT:
    case STT_FUNC:
      if (sym->shndx == SHN_UNDEF) {
        return -1;
      } else if (sym->shndx == SHN_ABS) {
        *result = sym->value;
        return 0;
      } else if (sym->shndx == SHN_COMMON) {
        return -1;
      } else {
        ElfuScn *newscn;

        assert (sym->scnptr);
        newscn = elfu_mScnByOldscn(me, sym->scnptr);
        assert(newscn);
        *result = newscn->shdr.sh_addr + sym->value;
        return 0;
      }
      break;
    case STT_SECTION:
      assert(sym->scnptr);
      assert(elfu_mScnByOldscn(me, sym->scnptr));
      *result = elfu_mScnByOldscn(me, sym->scnptr)->shdr.sh_addr;
      return 0;
    case STT_FILE:
      ELFU_WARN("elfu_mSymtabLookupSymToAddr: Symbol type FILE is not supported.\n");
      return -1;
    default:
      ELFU_WARN("elfu_mSymtabLookupSymToAddr: Unknown symbol type %d for %s.\n", sym->type, symname);
      return -1;
  }
}



char* elfu_mSymtabSymToName(ElfuScn *msst, ElfuSym *sym)
{
  assert(msst);
  assert(sym);

  return ELFU_SYMSTR(msst, sym->name);
}



ElfuSym* elfu_mSymtabIndexToSym(ElfuScn *msst, GElf_Word entry)
{
  GElf_Word i;
  ElfuSym *sym;

  assert(msst);
  assert(entry > 0);
  assert(!CIRCLEQ_EMPTY(&msst->symtab.syms));

  sym = CIRCLEQ_FIRST(&msst->symtab.syms);
  for (i = 1; i < entry; i++) {
    sym = CIRCLEQ_NEXT(sym, elem);
  }

  return sym;
}



/* Look up a value in the symbol table section *msst which is in *me. */
GElf_Addr elfu_mSymtabLookupAddrByName(ElfuElf *me, ElfuScn *msst, char *name)
{
  ElfuSym *sym;

  assert(me);
  assert(msst);
  assert(name);
  assert(strlen(name) > 0);
  assert(!CIRCLEQ_EMPTY(&msst->symtab.syms));

  CIRCLEQ_FOREACH(sym, &msst->symtab.syms, elem) {
    char *symname = ELFU_SYMSTR(msst, sym->name);

    if (!strcmp(symname, name)) {
      goto SYMBOL_FOUND;
    }
  }
  return 0;


  SYMBOL_FOUND:

  switch (sym->type) {
    case STT_NOTYPE:
    case STT_OBJECT:
    case STT_FUNC:
      if (sym->scnptr) {
        GElf_Addr a = sym->value;
        a += me->ehdr.e_type == ET_REL ? sym->scnptr->shdr.sh_addr : 0;
        return a;
      } else if (sym->shndx == SHN_UNDEF) {
        return 0;
      } else if (sym->shndx == SHN_ABS) {
        return sym->value;
      } else {
        ELFU_WARN("elfu_mSymtabLookupAddrByName: Symbol binding COMMON is not supported, using 0.\n");
        return 0;
      }
      break;
    default:
      return 0;
  }
}



/* Convert symtab from memory model to elfclass specific format */
void elfu_mSymtabFlatten(ElfuElf *me)
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

    if (me->symtab->databuf) {
      free(me->symtab->databuf);
    }
    me->symtab->databuf = malloc(newsize);
    assert(me->symtab->databuf);

    me->symtab->shdr.sh_size = newsize;
    memset(me->symtab->databuf, 0, newsize);

    i = 1;
    CIRCLEQ_FOREACH(sym, &me->symtab->symtab.syms, elem) {
      Elf32_Sym *es = ((Elf32_Sym*)me->symtab->databuf) + i;

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

    if (me->symtab->databuf) {
      free(me->symtab->databuf);
    }
    me->symtab->databuf = malloc(newsize);
    assert(me->symtab->databuf);

    me->symtab->shdr.sh_size = newsize;
    memset(me->symtab->databuf, 0, newsize);

    i = 1;
    CIRCLEQ_FOREACH(sym, &me->symtab->symtab.syms, elem) {
      Elf64_Sym *es = ((Elf64_Sym*)me->symtab->databuf) + i;

      es->st_name = sym->name;
      es->st_value = sym->value;
      es->st_size = sym->size;
      es->st_info = ELF64_ST_INFO(sym->bind, sym->type);
      es->st_other = sym->other;
      es->st_shndx = sym->shndx;

      i++;
    }
  } else {
    /* Unknown elfclass */
    assert(0);
  }

  elfu_mLayoutAuto(me);
}
