#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


/* Apply relocation information from section *msrt to data in
 * section *mstarget (which is stored in *metarget). */
int elfu_mRelocate(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt)
{
  ElfuRel *rel;
  ElfuSym *sym;

  assert(mstarget);
  assert(msrt);

  ELFU_DEBUG("Relocating in section of type %u size %x\n",
             mstarget->shdr.sh_type,
             (unsigned)mstarget->shdr.sh_size);

  CIRCLEQ_FOREACH(rel, &msrt->reltab.rels, elem) {
    Elf32_Word *dest32 = (Elf32_Word*)(mstarget->databuf + rel->offset);
    Elf64_Word *dest64 = (Elf64_Word*)(mstarget->databuf + rel->offset);
    GElf_Addr s;
    int haveSymval = 0;

    sym = elfu_mSymtabIndexToSym(msrt->linkptr, rel->sym);
    assert(sym);

    haveSymval = !elfu_mSymtabLookupSymToAddr(metarget,
                                              msrt->linkptr,
                                              sym,
                                              &s);
    if (!haveSymval) {
      if (sym->shndx == SHN_UNDEF) {
        haveSymval = !elfu_mDynLookupPltAddrByName(metarget,
                                     elfu_mSymtabSymToName(msrt->linkptr, sym),
                                     &s);
        if (!haveSymval) {
          haveSymval = !elfu_mDynLookupReldynAddrByName(metarget,
                                       elfu_mSymtabSymToName(msrt->linkptr, sym),
                                       &s);
        }
      }
    }

    if (metarget->elfclass == ELFCLASS32) {
      Elf32_Word a32 = rel->addendUsed ? rel->addend : *dest32;
      Elf32_Addr p32 = mstarget->shdr.sh_addr + rel->offset;
      Elf32_Addr s32 = (Elf32_Addr)s;

      switch(metarget->ehdr.e_machine) {
        case EM_386:
          switch(rel->type) {
            case R_386_NONE:
              ELFU_DEBUG("Skipping relocation: R_386_NONE\n");
              break;
            case R_386_32:
              if (!haveSymval) {
                goto MISSINGSYM;
              }
              *dest32 = s32 + a32;
              break;
            case R_386_PC32:
              if (!haveSymval) {
                goto MISSINGSYM;
              }
              *dest32 = s32 + a32 - p32;
              break;
            default:
              ELFU_DEBUG("elfu_mRelocate: Skipping unknown relocation type %d\n", rel->type);
          }
          break;
        default:
          ELFU_WARN("elfu_mRelocate: Unknown machine type. Aborting.\n");
      }
    } else if (metarget->elfclass == ELFCLASS64) {
      Elf64_Word a64 = rel->addendUsed ? rel->addend : *dest64;
      Elf64_Addr p64 = mstarget->shdr.sh_addr + rel->offset;
      Elf64_Addr s64 = (Elf64_Addr)s;

      switch(metarget->ehdr.e_machine) {
        case EM_X86_64:
          switch(rel->type) {
            case R_X86_64_NONE:
              ELFU_DEBUG("Skipping relocation: R_386_NONE\n");
              break;
            case R_X86_64_64:
              if (!haveSymval) {
                goto MISSINGSYM;
              }
              *dest64 = s64 + a64;
              break;
            case R_X86_64_PC32:
              if (!haveSymval) {
                goto MISSINGSYM;
              }
              *dest32 = s64 + a64 - p64;
              break;
            case R_X86_64_32:
              if (!haveSymval) {
                goto MISSINGSYM;
              }
              *dest32 = s64 + a64;
              break;
            default:
              ELFU_DEBUG("elfu_mRelocate: Skipping unknown relocation type %d", rel->type);
          }
          break;
        default:
          ELFU_WARN("elfu_mRelocate: Unknown machine type. Aborting.\n");
      }
    }
  }

  return 0;

  MISSINGSYM:
  ELFU_WARN("elfu_mRelocate: Could not resolve symbol %s. Aborting.\n",
            elfu_mSymtabSymToName(msrt->linkptr, sym));
  return -1;

}
