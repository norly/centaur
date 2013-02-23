#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libelf.h>
#include <gelf.h>
#include <libelfu/libelfu.h>


static ElfuPhdr* modelFromPhdr(GElf_Phdr *phdr)
{
  ElfuPhdr *mp;

  mp = malloc(sizeof(ElfuPhdr));
  if (!mp) {
    return NULL;
  }

  mp->phdr = *phdr;

  return mp;
}


static ElfuScn* modelFromSection(Elf_Scn *scn)
{
  ElfuScn *ms;
  Elf_Data *data;

  ms = malloc(sizeof(ElfuScn));
  if (!ms) {
    return NULL;
  }

  CIRCLEQ_INIT(&ms->dataList);


  if (gelf_getshdr(scn, &ms->shdr) != &ms->shdr) {
    fprintf(stderr, "gelf_getshdr() failed: %s\n", elf_errmsg(-1));
    goto out;
  }



  /* Copy each data part in source segment */
  data = elf_rawdata(scn, NULL);
  while (data) {
    ElfuData *md;

    md = malloc(sizeof(ElfuData));
    if (!md) {
      goto out;
    }

    md->data = *data;

    CIRCLEQ_INSERT_TAIL(&ms->dataList, md, elem);

    data = elf_rawdata(scn, data);
  }

  return ms;

  out:
  // FIXME
  return NULL;
}




ElfuElf* elfu_mFromElf(Elf *e)
{
  ElfuElf *me;
  Elf_Scn *scn;
  size_t shstrndx;
  size_t i, n;

  if (elf_getshdrstrndx(e, &shstrndx) != 0) {
    shstrndx = 0;
  }

  me = malloc(sizeof(ElfuElf));
  if (!me) {
    return NULL;
  }

  CIRCLEQ_INIT(&me->scnList);
  CIRCLEQ_INIT(&me->phdrList);
  me->shstrtab = NULL;

  /*
   * General stuff
   */
  me->elfclass = gelf_getclass(e);
  if (me->elfclass == ELFCLASSNONE) {
    fprintf(stderr, "getclass() failed: %s\n", elf_errmsg(-1));
  }

  if (!gelf_getehdr(e, &me->ehdr)) {
    fprintf(stderr, "gelf_getehdr() failed: %s\n", elf_errmsg(-1));
    goto out;
  }


  /*
   * Sections
   */
  scn = elf_getscn(e, 1);
  i = 1;
  while (scn) {
    ElfuScn *ms = modelFromSection(scn);

    if (ms) {
      CIRCLEQ_INSERT_TAIL(&me->scnList, ms, elem);
      if (i == shstrndx) {
        me->shstrtab = ms;
      }
    } else {
      goto out;
    }

    scn = elf_nextscn(e, scn);
    i++;
  }



  /*
   * Segments
   */
  if (elf_getphdrnum(e, &n)) {
    fprintf(stderr, "elf_getphdrnum() failed: %s\n", elf_errmsg(-1));
  }

  for (i = 0; i < n; i++) {
    GElf_Phdr phdr;
    ElfuPhdr *mp;

    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      fprintf(stderr, "gelf_getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
      break;
    }

    mp = modelFromPhdr(&phdr);

    if (mp) {
      CIRCLEQ_INSERT_TAIL(&me->phdrList, mp, elem);
    } else {
      goto out;
    }
  }



  return me;

  out:
  // FIXME
  return NULL;
}
