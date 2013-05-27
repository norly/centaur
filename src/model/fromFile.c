#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>


static int cmpScnOffs(const void *ms1, const void *ms2)
{
  assert(ms1);
  assert(ms2);

  ElfuScn *s1 = *(ElfuScn**)ms1;
  ElfuScn *s2 = *(ElfuScn**)ms2;

  assert(s1);
  assert(s2);


  if (s1->shdr.sh_offset < s2->shdr.sh_offset) {
    return -1;
  } else if (s1->shdr.sh_offset == s2->shdr.sh_offset) {
    return 0;
  } else /* if (s1->shdr.sh_offset > s2->shdr.sh_offset) */ {
    return 1;
  }
}



static ElfuPhdr* parentPhdr(ElfuElf *me, ElfuScn *ms)
{
  ElfuPhdr *mp;

  assert(me);
  assert(ms);

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (ms->shdr.sh_addr >= mp->phdr.p_vaddr
        && OFFS_END(ms->shdr.sh_addr, ms->shdr.sh_size) <= OFFS_END(mp->phdr.p_vaddr, mp->phdr.p_memsz)) {
      return mp;
    } else if (ms->shdr.sh_offset >= mp->phdr.p_offset
               && OFFS_END(ms->shdr.sh_offset, SCNFILESIZE(&ms->shdr)) <= OFFS_END(mp->phdr.p_offset, mp->phdr.p_filesz)
               && OFFS_END(ms->shdr.sh_offset, ms->shdr.sh_size) <= OFFS_END(mp->phdr.p_offset, mp->phdr.p_memsz)) {
      return mp;
    }
  }

  return NULL;
}


static ElfuPhdr* modelFromPhdr(GElf_Phdr *phdr)
{
  ElfuPhdr *mp;

  assert(phdr);

  mp = malloc(sizeof(ElfuPhdr));
  if (!mp) {
    ELFU_WARN("modelFromPhdr: malloc() failed for ElfuPhdr.\n");
    return NULL;
  }

  mp->phdr = *phdr;

  CIRCLEQ_INIT(&mp->phdrToScnList);

  return mp;
}


static ElfuScn* modelFromSection(Elf_Scn *scn)
{
  ElfuScn *ms;

  assert(scn);

  ms = malloc(sizeof(ElfuScn));
  if (!ms) {
    ELFU_WARN("modelFromSection: malloc() failed for ElfuScn.\n");
    goto ERROR;
  }


  assert(gelf_getshdr(scn, &ms->shdr) == &ms->shdr);


  /* Copy each data part in source segment */
  ms->data.d_align = 1;
  ms->data.d_buf  = NULL;
  ms->data.d_off  = 0;
  ms->data.d_type = ELF_T_BYTE;
  ms->data.d_size = ms->shdr.sh_size;
  ms->data.d_version = elf_version(EV_NONE);
  if (ms->shdr.sh_type != SHT_NOBITS
      && ms->shdr.sh_size > 0) {
    Elf_Data *data;

    ms->data.d_buf = malloc(ms->shdr.sh_size);
    if (!ms->data.d_buf) {
      ELFU_WARN("modelFromSection: malloc() failed for data buffer (%jx bytes).\n", ms->shdr.sh_size);
      goto ERROR;
    }

    /* A non-empty section should contain at least one data block. */
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

  ms->linkptr = NULL;
  ms->infoptr = NULL;


  return ms;

  ERROR:
  if (ms) {
    free(ms);
  }
  return NULL;
}




ElfuElf* elfu_mFromElf(Elf *e)
{
  ElfuElf *me;
  size_t shstrndx;
  size_t i, numPhdr, numShdr;
  ElfuScn **secArray = NULL;

  assert(e);
  if (elfu_eCheck(e)) {
    goto ERROR;
  }

  me = malloc(sizeof(ElfuElf));
  if (!me) {
    ELFU_WARN("elfu_mFromElf: malloc() failed for ElfuElf.\n");
    goto ERROR;
  }


  /* General stuff */
  CIRCLEQ_INIT(&me->scnList);
  CIRCLEQ_INIT(&me->phdrList);
  me->shstrtab = NULL;

  me->elfclass = gelf_getclass(e);
  assert(me->elfclass != ELFCLASSNONE);
  assert(gelf_getehdr(e, &me->ehdr) == &me->ehdr);


  /* Get the section string table index */
  if (elf_getshdrstrndx(e, &shstrndx) != 0) {
    shstrndx = 0;
  }


  /* Load segments */
  assert(!elf_getphdrnum(e, &numPhdr));
  for (i = 0; i < numPhdr; i++) {
    GElf_Phdr phdr;
    ElfuPhdr *mp;

    assert(gelf_getphdr(e, i, &phdr) == &phdr);

    mp = modelFromPhdr(&phdr);
    if (!mp) {
      goto ERROR;
    }

    CIRCLEQ_INSERT_TAIL(&me->phdrList, mp, elem);
  }


  /* Load sections */
  assert(!elf_getshdrnum(e, &numShdr));
  if (numShdr > 1) {
    secArray = malloc((numShdr - 1) * sizeof(*secArray));
    if (!secArray) {
      ELFU_WARN("elfu_mFromElf: malloc() failed for secArray.\n");
      goto ERROR;
    }

    for (i = 1; i < numShdr; i++) {
      Elf_Scn *scn;
      ElfuScn *ms;

      scn = elf_getscn(e, i);
      assert(scn);

      ms = modelFromSection(scn);
      if (!ms) {
        goto ERROR;
      }

      secArray[i-1] =  ms;

      if (i == shstrndx) {
        me->shstrtab = ms;
      }
    }


    /* Find sh_link dependencies */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      switch (ms->shdr.sh_type) {
        case SHT_REL:
        case SHT_RELA:
          if (ms->shdr.sh_info > 0) {
            ms->infoptr = secArray[ms->shdr.sh_info - 1];
          }
        case SHT_DYNAMIC:
        case SHT_HASH:
        case SHT_SYMTAB:
        case SHT_DYNSYM:
        case SHT_GNU_versym:
        case SHT_GNU_verdef:
        case SHT_GNU_verneed:
          if (ms->shdr.sh_link > 0) {
            ms->linkptr = secArray[ms->shdr.sh_link - 1];
          }
      }
    }


    /* Sort sections by file offset */
    qsort(secArray, numShdr - 1, sizeof(*secArray), cmpScnOffs);


    /* Find PHDR -> Section dependencies (needs sorted sections) */
    for (i = 0; i < numShdr - 1; i++) {
      ElfuScn *ms = secArray[i];

      ElfuPhdr *parent = parentPhdr(me, ms);

      if (parent) {
        CIRCLEQ_INSERT_TAIL(&parent->phdrToScnList, ms, elemPhdrToScn);
      }
    }


    /* Put sections into list of all sections */
    for (i = 0; i < numShdr - 1; i++) {
      CIRCLEQ_INSERT_TAIL(&me->scnList, secArray[i], elem);
    }
  }


  return me;


  ERROR:
  if (secArray) {
    free(secArray);
  }
  if (me) {
    // TODO: Free data structures
  }

  ELFU_WARN("elfu_mFromElf: Failed to load file.\n");
  return NULL;
}
