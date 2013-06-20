#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


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
  Elf_Scn *scnOut;
  Elf *e = (Elf*)aux1;
  (void) me;
  (void) aux2;

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
  if (ms->databuf) {
    Elf_Data *dataOut = elf_newdata(scnOut);
    if (!dataOut) {
      ELFU_WARNELF("elf_newdata");
    }

    dataOut->d_align = 1;
    dataOut->d_buf  = ms->databuf;
    dataOut->d_off  = 0;
    dataOut->d_type = ELF_T_BYTE;
    dataOut->d_size = ms->shdr.sh_size;
    dataOut->d_version = elf_version(EV_NONE);
  }

  return NULL;
}





void elfu_mToElf(ElfuElf *me, Elf *e)
{
  if (me->symtab) {
    elfu_mSymtabFlatten(me);
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
