#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>


static int appendData(ElfuScn *ms, void *buf, size_t len)
{
  void *newbuf;

  assert(ms);
  assert(ms->shdr.sh_type != SHT_NOBITS);
  assert(ms->data.d_buf);

  newbuf = realloc(ms->data.d_buf, ms->shdr.sh_size + len);
  if (!newbuf) {
    ELFU_WARN("appendData: malloc() failed for newbuf.\n");
    return 1;
  }

  ms->data.d_buf = newbuf;
  memcpy(newbuf + ms->shdr.sh_size, buf, len);
  ms->shdr.sh_size += len;
  ms->data.d_size += len;
  assert(ms->shdr.sh_size == ms->data.d_size);

  return 0;
}


static ElfuScn* insertSection(ElfuElf *me, ElfuElf *mrel, ElfuScn *oldscn)
{
  ElfuScn *newscn = NULL;
  GElf_Addr injAddr;
  GElf_Off injOffset;
  ElfuPhdr *injPhdr;

  if (oldscn->shdr.sh_flags & SHF_ALLOC) {
    newscn = elfu_mCloneScn(oldscn);
    if (!newscn) {
      return NULL;
    }

    if (newscn->shdr.sh_type == SHT_NOBITS) {
      /* Expand this to SHT_PROGBITS, then insert as such. */

      assert(!newscn->data.d_buf);

      newscn->data.d_buf = malloc(newscn->shdr.sh_size);
      if (!newscn->data.d_buf) {
        goto ERROR;
      }
      newscn->data.d_size = newscn->shdr.sh_size;
      newscn->shdr.sh_type = SHT_PROGBITS;
    }

    injAddr = elfu_mLayoutGetSpaceInPhdr(me,
                                         newscn->shdr.sh_size,
                                         newscn->shdr.sh_addralign,
                                         newscn->shdr.sh_flags & SHF_WRITE,
                                         newscn->shdr.sh_flags & SHF_EXECINSTR,
                                         &injPhdr);

    if (!injPhdr) {
      ELFU_WARN("insertSection: Could not find a place to insert section.\n");
      goto ERROR;
    }

    ELFU_INFO("Inserting %s at address 0x%jx...\n",
              elfu_mScnName(mrel, oldscn),
              injAddr);

    injOffset = injAddr - injPhdr->phdr.p_vaddr + injPhdr->phdr.p_offset;

    newscn->shdr.sh_addr = injAddr;
    newscn->shdr.sh_offset = injOffset;

    if (CIRCLEQ_EMPTY(&injPhdr->childScnList)
        || CIRCLEQ_LAST(&injPhdr->childScnList)->shdr.sh_offset < injOffset) {
      CIRCLEQ_INSERT_TAIL(&injPhdr->childScnList, newscn, elemChildScn);
    } else {
      ElfuScn *ms;
      CIRCLEQ_FOREACH(ms, &injPhdr->childScnList, elemChildScn) {
        if (injOffset < ms->shdr.sh_offset) {
          CIRCLEQ_INSERT_BEFORE(&injPhdr->childScnList, ms, newscn, elemChildScn);
          break;
        }
      }
    }


    /* Inject name */
    if (me->shstrtab) {
      char *newname;
      size_t newnamelen;

      newnamelen = strlen("reladd") + 1;
      if (elfu_mScnName(mrel, oldscn)) {
        newnamelen += strlen(elfu_mScnName(mrel, oldscn));
      }

      newname = malloc(newnamelen);
      strcpy(newname, "reladd");
      strcat(newname, elfu_mScnName(mrel, oldscn));

      if (!newname) {
        ELFU_WARN("insertSection: malloc() failed for newname. Leaving section name empty.\n");
        newscn->shdr.sh_name = 0;
      } else {
        size_t offset = me->shstrtab->shdr.sh_size;

        if (!appendData(me->shstrtab, newname, newnamelen)) {
          newscn->shdr.sh_name = offset;
        }

        free(newname);
      }
    }

    return newscn;
  } else {
      ELFU_WARN("insertSection: Skipping non-memory section %s (type %d flags %jd).\n",
                elfu_mScnName(mrel, oldscn),
                oldscn->shdr.sh_type,
                oldscn->shdr.sh_flags);
      goto ERROR;
  }

  ERROR:
  if (newscn) {
    // TODO: Destroy newscn
  }
  return NULL;
}


static void* subScnAdd1(ElfuElf *mrel, ElfuScn *ms, void *aux1, void *aux2)
{
  (void)aux2;
  ElfuElf *me = (ElfuElf*)aux1;

  ElfuScn *newscn;

  switch(ms->shdr.sh_type) {
    case SHT_PROGBITS: /* 1 */
    case SHT_NOBITS: /* 8 */
      /* Ignore empty sections */
      if (ms->shdr.sh_size == 0) {
        break;
      }

      /* Find a place where it belongs and shove it in. */
      newscn = insertSection(me, mrel, ms);
      if (!newscn) {
        ELFU_WARN("mReladd: Could not insert section %s (type %d), skipping.\n",
                  elfu_mScnName(mrel, ms),
                  ms->shdr.sh_type);
      }
      break;
  }

  return NULL;
}


static void* subScnAdd2(ElfuElf *mrel, ElfuScn *ms, void *aux1, void *aux2)
{
  (void)aux2;
  ElfuElf *me = (ElfuElf*)aux1;
  (void)me;

  switch(ms->shdr.sh_type) {
    case SHT_NULL: /* 0 */
    case SHT_PROGBITS: /* 1 */
    case SHT_STRTAB: /* 3 */
    case SHT_NOBITS: /* 8 */
      break;


    case SHT_REL: /* 9 */
      /* Relocate. */
      elfu_mRelocate32(me, elfu_mScnByOldscn(me, ms->infoptr), ms);
      break;

    case SHT_RELA: /* 4 */
      // TODO: Needs a parser
      //elfu_mRelocate(elfu_mScnByOldscn(me, ms->infoptr), ms);

    case SHT_SYMTAB: /* 2 */
      /* Merge with the existing table. Take care of string tables also. */

    /* The next section types either do not occur in .o files, or are
     * not strictly necessary to process here. */
    case SHT_NOTE: /* 7 */
    case SHT_HASH: /* 5 */
    case SHT_DYNAMIC: /* 6 */
    case SHT_SHLIB: /* 10 */
    case SHT_DYNSYM: /* 11 */
    case SHT_INIT_ARRAY: /* 14 */
    case SHT_FINI_ARRAY: /* 15 */
    case SHT_PREINIT_ARRAY: /* 16 */
    case SHT_GROUP: /* 17 */
    case SHT_SYMTAB_SHNDX: /* 18 */
    case SHT_NUM: /* 19 */
    default:
      ELFU_WARN("mReladd: Skipping section %s (type %d).\n",
                elfu_mScnName(mrel, ms),
                ms->shdr.sh_type);
  }

  return NULL;
}


void elfu_mReladd(ElfuElf *me, ElfuElf *mrel)
{
  assert(me);
  assert(mrel);

  /* For each section in object file, guess how to insert it */
  elfu_mScnForall(mrel, subScnAdd1, me, NULL);

  /* Do relocations and other stuff */
  elfu_mScnForall(mrel, subScnAdd2, me, NULL);

  /* Re-layout to accommodate new contents */
  elfu_mLayoutAuto(me);
}
