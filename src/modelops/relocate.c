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
static GElf_Word pltLookupVal(ElfuElf *metarget, char *name)
{
  ElfuScn *relplt;
  ElfuScn *plt;
  ElfuRel *rel;
  GElf_Word j;

  relplt = elfu_mScnForall(metarget, subFindByName, ".rel.plt", NULL);
  if (!relplt) {
    ELFU_WARN("dynsymLookupVal: Could not find .rel.plt section in destination ELF.\n");
    return 0;
  }

  plt = elfu_mScnForall(metarget, subFindByName, ".plt", NULL);
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
    if ((metarget->elfclass == ELFCLASS32 && rel->type != R_386_JMP_SLOT)
        || (metarget->elfclass == ELFCLASS64 && rel->type != R_X86_64_JUMP_SLOT)) {
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
      ELFU_DEBUG("dynsymLookupVal: Guessing symbol '%s' is in destination memory at %jx (PLT entry #%d).\n", name, addr, j);
      return addr;
    }
  }

  ELFU_WARN("dynsymLookupVal: Could not find symbol '%s' in destination ELF.\n", name);

  return 0;
}


static GElf_Word symtabLookupVal(ElfuElf *metarget, ElfuScn *msst, GElf_Word entry)
{
  GElf_Word i;
  ElfuSym *sym;
  char *symname;

  assert(metarget);
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
        ElfuScn *newscn = elfu_mScnByOldscn(metarget, sym->scnptr);
        assert(newscn);
        return newscn->shdr.sh_addr + sym->value;
      } else if (sym->shndx == SHN_UNDEF) {
        /* Look the symbol up in .rel.plt. If it cannot be found there then
         * .rel.dyn may need to be expanded with a COPY relocation so the
         * dynamic linker fixes up the (TODO). */
        return pltLookupVal(metarget, symname);
      } else if (sym->shndx == SHN_ABS) {
        return sym->value;
      } else {
        ELFU_WARN("symtabLookupVal: Symbol binding COMMON is not supported, using 0.\n");
        return 0;
      }
      break;
    case STT_SECTION:
      assert(sym->scnptr);
      assert(elfu_mScnByOldscn(metarget, sym->scnptr));
      return elfu_mScnByOldscn(metarget, sym->scnptr)->shdr.sh_addr;
    case STT_FILE:
      ELFU_WARN("symtabLookupVal: Symbol type FILE is not supported, using 0.\n");
      return 0;
    default:
      ELFU_WARN("symtabLookupVal: Unknown symbol type %d for %s.\n", sym->type, symname);
      return 0;
  }
}

void elfu_mRelocate(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt)
{
  ElfuRel *rel;

  assert(mstarget);
  assert(msrt);

  ELFU_DEBUG("Relocating in section of type %d size %jx\n",
             mstarget->shdr.sh_type,
             mstarget->shdr.sh_size);

  CIRCLEQ_FOREACH(rel, &msrt->reltab.rels, elem) {
    Elf32_Word *dest32 = (Elf32_Word*)(((char*)(mstarget->data.d_buf)) + rel->offset);
    Elf64_Word *dest64 = (Elf64_Word*)(((char*)(mstarget->data.d_buf)) + rel->offset);


    if (metarget->elfclass == ELFCLASS32) {
      Elf32_Word a32 = rel->addendUsed ? rel->addend : *dest32;
      Elf32_Addr p32 = mstarget->shdr.sh_addr + rel->offset;
      Elf32_Addr s32 = symtabLookupVal(metarget, msrt->linkptr, rel->sym);
      switch(rel->type) {
        case R_386_NONE:
          ELFU_DEBUG("Skipping relocation: R_386_NONE\n");
          break;
        case R_386_32:
          *dest32 = s32 + a32;
          break;
        case R_386_PC32:
          *dest32 = s32 + a32 - p32;
          break;

        default:
          ELFU_DEBUG("Skipping relocation: Unknown type %d\n", rel->type);
      }
    } else if (metarget->elfclass == ELFCLASS64) {
      /* x86-64 only uses RELA with explicit addend. */
      assert(rel->addendUsed);
      Elf64_Word a64 = rel->addend;
      Elf64_Addr p64 = mstarget->shdr.sh_addr + rel->offset;
      Elf64_Addr s64 = symtabLookupVal(metarget, msrt->linkptr, rel->sym);

      switch(rel->type) {
        case R_X86_64_NONE:
          ELFU_DEBUG("Skipping relocation: R_386_NONE\n");
          break;
        case R_X86_64_64:
          *dest64 = s64 + a64;
          break;
        case R_X86_64_PC32:
          *dest32 = s64 + a64 - p64;
          break;
        case R_X86_64_32:
          *dest32 = s64 + a64;
          break;

        default:
          ELFU_DEBUG("Skipping relocation: Unknown type %d", rel->type);
      }
    }
  }
}
