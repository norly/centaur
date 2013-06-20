#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


/* Apply relocation information from section *msrt to data in
 * section *mstarget (which is stored in *metarget). */
void elfu_mRelocate(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt)
{
  ElfuRel *rel;

  assert(mstarget);
  assert(msrt);

  ELFU_DEBUG("Relocating in section of type %u size %x\n",
             mstarget->shdr.sh_type,
             (unsigned)mstarget->shdr.sh_size);

  CIRCLEQ_FOREACH(rel, &msrt->reltab.rels, elem) {
    Elf32_Word *dest32 = (Elf32_Word*)(((char*)(mstarget->data.d_buf)) + rel->offset);
    Elf64_Word *dest64 = (Elf64_Word*)(((char*)(mstarget->data.d_buf)) + rel->offset);


    if (metarget->elfclass == ELFCLASS32) {
      Elf32_Word a32 = rel->addendUsed ? rel->addend : *dest32;
      Elf32_Addr p32 = mstarget->shdr.sh_addr + rel->offset;
      Elf32_Addr s32 = elfu_mSymtabLookupVal(metarget, msrt->linkptr, rel->sym);
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
      Elf64_Word a64 = rel->addend;
      Elf64_Addr p64 = mstarget->shdr.sh_addr + rel->offset;
      Elf64_Addr s64 = elfu_mSymtabLookupVal(metarget, msrt->linkptr, rel->sym);

      /* x86-64 only uses RELA with explicit addend. */
      assert(rel->addendUsed);

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
