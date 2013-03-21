#include <stdio.h>
#include <stdlib.h>
#include <libelf.h>
#include <gelf.h>
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
    fprintf(stderr, "gelf_newphdr() failed: %s\n", elf_errmsg(-1));
  }

  /* Copy PHDRs */
  i = 0;
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (!gelf_update_phdr (e, i, &mp->phdr)) {
      fprintf(stderr, "gelf_update_phdr() failed: %s\n", elf_errmsg(-1));
    }

    i++;
  }
}



static void modelToSection(ElfuScn *ms, Elf *e)
{
  Elf_Scn *scnOut;

  scnOut = elf_newscn(e);
  if (!scnOut) {
    fprintf(stderr, "elf_newscn() failed: %s\n", elf_errmsg(-1));
    return;
  }


  /* SHDR */
  if (!gelf_update_shdr(scnOut, &ms->shdr)) {
    fprintf(stderr, "gelf_update_shdr() failed: %s\n", elf_errmsg(-1));
  }


  /* Data */
  if (ms->data.d_buf) {
    Elf_Data *dataOut = elf_newdata(scnOut);
    if (!dataOut) {
      fprintf(stderr, "elf_newdata() failed: %s\n", elf_errmsg(-1));
    }

    dataOut->d_align = ms->data.d_align;
    dataOut->d_buf  = ms->data.d_buf;
    dataOut->d_off  = ms->data.d_off;
    dataOut->d_type = ms->data.d_type;
    dataOut->d_size = ms->data.d_size;
    dataOut->d_version = ms->data.d_version;
  }
}





void elfu_mToElf(ElfuElf *me, Elf *e)
{
  ElfuScn *ms;

  /* We control the ELF file's layout now. */
  /* tired's libelf also offers ELF_F_LAYOUT_OVERLAP for overlapping sections. */
  elf_flagelf(e, ELF_C_SET, ELF_F_LAYOUT);


  /* EHDR */
  if (!gelf_newehdr(e, me->elfclass)) {
    fprintf(stderr, "gelf_newehdr() failed: %s\n", elf_errmsg(-1));
  }

  if (!gelf_update_ehdr(e, &me->ehdr)) {
    fprintf(stderr, "gelf_update_ehdr() failed: %s\n", elf_errmsg(-1));
  }


  /* Sections */
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    modelToSection(ms, e);
  }


  /* PHDRs */
  modelToPhdrs(me, e);


  elf_flagelf(e, ELF_C_SET, ELF_F_DIRTY);
}
