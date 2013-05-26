#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>
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


  ms = malloc(sizeof(ElfuScn));
  if (!ms) {
    return NULL;
  }


  if (gelf_getshdr(scn, &ms->shdr) != &ms->shdr) {
    ELFU_WARN("gelf_getshdr() failed: %s\n", elf_errmsg(-1));
    goto out;
  }


  /* Copy each data part in source segment */
  ms->data.d_align = 1;
  ms->data.d_buf  = NULL;
  ms->data.d_off  = 0;
  ms->data.d_type = ELF_T_BYTE;
  ms->data.d_size = ms->shdr.sh_size;
  ms->data.d_version = elf_version(EV_NONE);
  if (ms->shdr.sh_type != SHT_NOBITS
      && ms->shdr.sh_size > 1) {
    Elf_Data *data;

    ms->data.d_buf = malloc(ms->shdr.sh_size);
    if (!ms->data.d_buf) {
      ELFU_WARN("modelFromSection: Could not allocate data buffer (%jx bytes).\n", ms->shdr.sh_size);
      goto out;
    }

    /* A non-empty section should contain at least on data block. */
    data = elf_rawdata(scn, NULL);

    assert(data);

    ms->data.d_align = data->d_align;
    ms->data.d_type = data->d_type;
    ms->data.d_version = data->d_version;

    while (data) {
      if (data->d_off + data->d_size > ms->shdr.sh_size) {
        ELFU_WARN("modelFromSection: libelf delivered a bogus data blob. Skipping\n");
      } else {
        memcpy(ms->data.d_buf + data->d_off, data->d_buf, data->d_size);
      }

      data = elf_rawdata(scn, data);
    }
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

  assert(e);
  if (elfu_eCheck(e)) {
    goto ERROR;
  }

  /* Get the section string table index */
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
    ELFU_WARNELF("getclass");
  }

  if (!gelf_getehdr(e, &me->ehdr)) {
    ELFU_WARNELF("gelf_getehdr");
    goto ERROR;
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
      goto ERROR;
    }

    scn = elf_nextscn(e, scn);
    i++;
  }



  /*
   * Segments
   */
  if (elf_getphdrnum(e, &n)) {
    ELFU_WARNELF("elf_getphdrnum");
  }

  for (i = 0; i < n; i++) {
    GElf_Phdr phdr;
    ElfuPhdr *mp;

    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      ELFU_WARN("gelf_getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
      break;
    }

    mp = modelFromPhdr(&phdr);

    if (mp) {
      CIRCLEQ_INSERT_TAIL(&me->phdrList, mp, elem);
    } else {
      goto ERROR;
    }
  }

  return me;


  ERROR:
  // TODO: Free data structures

  ELFU_WARN("elfu_mFromElf: Failed to load file.\n");
  return NULL;
}
