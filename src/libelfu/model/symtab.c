#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


static void* subFindByName(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  char *name = (char*)aux1;
  (void)aux2;

  if (elfu_mScnName(me, ms)) {
    if (!strcmp(elfu_mScnName(me, ms), name)) {
      return ms;
    }
  }

  /* Continue */
  return NULL;
}

/* Hazard a guess where a function may be found in the PLT */
static GElf_Word pltLookupVal(ElfuElf *me, char *name)
{
  ElfuScn *relplt;
  ElfuScn *plt;
  ElfuRel *rel;
  GElf_Word j;

  relplt = elfu_mScnForall(me, subFindByName, ".rel.plt", NULL);
  if (!relplt) {
    ELFU_WARN("dynsymLookupVal: Could not find .rel.plt section in destination ELF.\n");
    return 0;
  }

  plt = elfu_mScnForall(me, subFindByName, ".plt", NULL);
  if (!plt) {
    ELFU_WARN("dynsymLookupVal: Could not find .plt section in destination ELF.\n");
    return 0;
  }


  /* Look up name. If the j-th entry in .rel.plt has the name we are
   * looking for, we assume that the (j+1)-th entry in .plt is machine
   * code to jump to the function.
   * Your mileage may vary, but it works on my GNU binaries. */
  assert(relplt->linkptr);
  j = 0;
  CIRCLEQ_FOREACH(rel, &relplt->reltab.rels, elem) {
    GElf_Word i;
    ElfuSym *sym;
    char *symname;

    j++;

    /* We only consider runtime relocations for functions.
     * Technically, these relocations write the functions' addresses
     * to the GOT, not the PLT, after the dynamic linker has found
     * them. */
    if ((me->elfclass == ELFCLASS32 && rel->type != R_386_JMP_SLOT)
        || (me->elfclass == ELFCLASS64 && rel->type != R_X86_64_JUMP_SLOT)) {
      continue;
    }

    /* Get the (rel->sym)-th symbol from the symbol table that
     * .rel.plt points to. */
    sym = CIRCLEQ_FIRST(&relplt->linkptr->symtab.syms);
    for (i = 1; i < rel->sym; i++) {
      sym = CIRCLEQ_NEXT(sym, elem);
    }

    symname = ELFU_SYMSTR(relplt->linkptr, sym->name);
    if (!strcmp(symname, name)) {
      /* If this is the symbol we are looking for, then in an x86 binary
       * the jump to the dynamic symbol is probably at offset (j * 16)
       * from the start of the PLT, where j is the PLT entry and 16 is
       * the number of bytes the machine code in a PLT entry take. */
      GElf_Addr addr = plt->shdr.sh_addr + (16 * j);
      ELFU_DEBUG("dynsymLookupVal: Guessing symbol '%s' is in destination memory at %x (PLT entry #%u).\n", name, (unsigned)addr, j);
      return addr;
    }
  }

  ELFU_WARN("dynsymLookupVal: Could not find symbol '%s' in destination ELF.\n", name);

  return 0;
}



/* Look up a value in the symbol table section *msst which is in *me.
 * If it is not found there, see if we can find it in *me's PLT. */
GElf_Word elfu_mSymtabLookupVal(ElfuElf *me, ElfuScn *msst, GElf_Word entry)
{
  GElf_Word i;
  ElfuSym *sym;
  char *symname;

  assert(me);
  assert(msst);
  assert(entry > 0);
  assert(!CIRCLEQ_EMPTY(&msst->symtab.syms));

  sym = CIRCLEQ_FIRST(&msst->symtab.syms);
  for (i = 1; i < entry; i++) {
    sym = CIRCLEQ_NEXT(sym, elem);
  }
  symname = ELFU_SYMSTR(msst, sym->name);

  switch (sym->type) {
    case STT_NOTYPE:
    case STT_OBJECT:
    case STT_FUNC:
      if (sym->scnptr) {
        ElfuScn *newscn = elfu_mScnByOldscn(me, sym->scnptr);
        assert(newscn);
        return newscn->shdr.sh_addr + sym->value;
      } else if (sym->shndx == SHN_UNDEF) {
        /* Look the symbol up in .rel.plt. If it cannot be found there then
         * .rel.dyn may need to be expanded with a COPY relocation so the
         * dynamic linker fixes up the (TODO). */
        return pltLookupVal(me, symname);
      } else if (sym->shndx == SHN_ABS) {
        return sym->value;
      } else {
        ELFU_WARN("symtabLookupVal: Symbol binding COMMON is not supported, using 0.\n");
        return 0;
      }
      break;
    case STT_SECTION:
      assert(sym->scnptr);
      assert(elfu_mScnByOldscn(me, sym->scnptr));
      return elfu_mScnByOldscn(me, sym->scnptr)->shdr.sh_addr;
    case STT_FILE:
      ELFU_WARN("symtabLookupVal: Symbol type FILE is not supported, using 0.\n");
      return 0;
    default:
      ELFU_WARN("symtabLookupVal: Unknown symbol type %d for %s.\n", sym->type, symname);
      return 0;
  }
}


/* Look up a value in the symbol table section *msst which is in *me. */
GElf_Word elfu_mSymtabLookupAddrByName(ElfuElf *me, ElfuScn *msst, char *name)
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
