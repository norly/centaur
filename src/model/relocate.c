#include <assert.h>
#include <stdlib.h>
#include <libelfu/libelfu.h>


static GElf_Word symtabLookupVal(ElfuElf *metarget, ElfuScn *msst, GElf_Word entry)
{
  GElf_Word i;
  ElfuSym *sym;

  assert(metarget);
  assert(msst);
  assert(msst->symtab);
  assert(entry > 0);
  assert(!CIRCLEQ_EMPTY(&msst->symtab->syms));

  sym = CIRCLEQ_FIRST(&msst->symtab->syms);
  for (i = 1; i < entry; i++) {
    sym = CIRCLEQ_NEXT(sym, elem);
  }

  switch (sym->type) {
    case STT_NOTYPE:
    case STT_OBJECT:
    case STT_FUNC:
      if (sym->scnptr) {
        assert(elfu_mScnByOldscn(metarget, sym->scnptr));
        return elfu_mScnByOldscn(metarget, sym->scnptr)->shdr.sh_addr + sym->value;
      } else {
        // TODO: UNDEF, ABS, or COMMON
        ELFU_WARN("symtabLookupVal: Returning 0 for UNDEF, ABS, or COMMON symbol.\n");
      }
      break;
    case STT_SECTION:
      assert(sym->scnptr);
      assert(elfu_mScnByOldscn(metarget, sym->scnptr));
      return elfu_mScnByOldscn(metarget, sym->scnptr)->shdr.sh_addr;
    case STT_FILE:
      // TODO
      ELFU_WARN("symtabLookupVal: Returning 0 for FILE symbol.\n");
      break;
    default:
      ELFU_WARN("symtabLookupVal: Unknown symbol type %d.\n", sym->type);
      return 0;
  }
}

void elfu_mRelocate32(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt)
{
  ElfuRel *rel;

  assert(mstarget);
  assert(msrt);

  ELFU_DEBUG("Relocating in section of type %d size %jx\n",
             mstarget->shdr.sh_type,
             mstarget->shdr.sh_size);

  CIRCLEQ_FOREACH(rel, &msrt->reltab->rels, elem) {
    Elf32_Word *dest = (Elf32_Word*)(((char*)(mstarget->data.d_buf)) + rel->offset);
    Elf32_Word a = rel->addendUsed ? rel->addend : *dest;
    Elf32_Addr p = mstarget->shdr.sh_addr + rel->offset;
    Elf32_Addr s = symtabLookupVal(metarget, msrt->linkptr, rel->sym);
    Elf32_Word newval = *dest;

    switch(rel->type) {
      case R_386_NONE:
        ELFU_DEBUG("Skipping relocation: R_386_NONE");
        break;
      case R_386_32:
        ELFU_DEBUG("Relocation: R_386_32");
        newval = s + a;
        break;
      case R_386_PC32:
        ELFU_DEBUG("Relocation: R_386_PC32");
        newval = s + a - p;
        break;

      default:
        ELFU_DEBUG("Skipping relocation: Unknown type %d", rel->type);
    }
    ELFU_DEBUG(", overwriting %x with %x.\n", *dest, newval);
    *dest = newval;
  }
}
