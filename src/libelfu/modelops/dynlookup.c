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
int elfu_mDynLookupPltAddrByName(ElfuElf *me, char *name, GElf_Addr *result)
{
  ElfuScn *relplt;
  ElfuScn *plt;
  ElfuRel *rel;
  GElf_Word j;

  relplt = elfu_mScnForall(me, subFindByName, ".rel.plt", NULL);
  if (!relplt) {
    /* x86-64 uses .rela.plt instead */
    relplt = elfu_mScnForall(me, subFindByName, ".rela.plt", NULL);
  }
  if (!relplt) {
    ELFU_WARN("elfu_mDynLookupPltAddr: Could not find .rel.plt section in destination ELF.\n");
    return -1;
  }

  plt = elfu_mScnForall(me, subFindByName, ".plt", NULL);
  if (!plt) {
    ELFU_WARN("elfu_mDynLookupPltAddr: Could not find .plt section in destination ELF.\n");
    return -1;
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
      ELFU_DEBUG("elfu_mDynLookupPltAddr: Guessing symbol '%s' is in destination memory at %x (PLT entry #%u).\n", name, (unsigned)addr, j);
      *result = addr;
      return 0;
    }
  }

  ELFU_WARN("elfu_mDynLookupPltAddr: Could not find symbol '%s' in destination ELF.\n", name);

  return -1;
}
