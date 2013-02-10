#include <stdio.h>
#include <stdlib.h>

#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>



static void elfu_modelToPhdrs(ElfuElf *me, Elf *e)
{
  ElfuPhdr *mp;
  size_t i;

  /* Count PHDRs */
  i = 0;
  for (mp = me->phdrList.cqh_first;
        mp != (void *)&me->phdrList;
        mp = mp->elem.cqe_next) {
    i++;
  }

  if (!gelf_newphdr(e, i)) {
    fprintf(stderr, "gelf_newphdr() failed: %s\n", elf_errmsg(-1));
  }

  /* Copy PHDRs */
  i = 0;
  for (mp = me->phdrList.cqh_first;
        mp != (void *)&me->phdrList;
        mp = mp->elem.cqe_next) {
    if (!gelf_update_phdr (e, i, &mp->phdr)) {
      fprintf(stderr, "gelf_update_phdr() failed: %s\n", elf_errmsg(-1));
    }

    i++;
  }
}



static void elfu_modelToSection(ElfuScn *ms, Elf *e)
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
  ElfuData *md;
  for (md = ms->dataList.cqh_first;
        md != (void *)&ms->dataList;
        md = md->elem.cqe_next) {
    Elf_Data *dataOut = elf_newdata(scnOut);
    if (!dataOut) {
      fprintf(stderr, "elf_newdata() failed: %s\n", elf_errmsg(-1));
    }

    dataOut->d_align = md->data.d_align;
    dataOut->d_buf  = md->data.d_buf;
    dataOut->d_off  = md->data.d_off;
    dataOut->d_type = md->data.d_type;
    dataOut->d_size = md->data.d_size;
    dataOut->d_version = md->data.d_version;
  }
}





void elfu_modelToElf(ElfuElf *me, Elf *e)
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
  for (ms = me->scnList.cqh_first;
        ms != (void *)&me->scnList;
        ms = ms->elem.cqe_next) {
    elfu_modelToSection(ms, e);
  }


  /* PHDRs */
  elfu_modelToPhdrs(me, e);


  elf_flagelf(e, ELF_C_SET, ELF_F_DIRTY);
}
