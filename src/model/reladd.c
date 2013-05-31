#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>


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
    // TODO
    newscn->shdr.sh_name = 0;

    // TODO: Relocate

    return newscn;
  } else {
      ELFU_WARN("insertSection: Skipping section %s with flags %jd (type %d).\n",
                elfu_mScnName(mrel, oldscn),
                oldscn->shdr.sh_flags,
                oldscn->shdr.sh_type);
      goto ERROR;
  }

  ERROR:
  if (newscn) {
    // TODO: Destroy newscn
  }
  return NULL;
}


int subScnAdd1(ElfuElf *mrel, ElfuScn *ms, void *aux1, void *aux2)
{
  (void)aux2;
  ElfuElf *me = (ElfuElf*)aux1;

  ElfuScn *newscn;

  switch(ms->shdr.sh_type) {
    case SHT_PROGBITS: /* 1 */
      /* Find a place where it belongs and shove it in. */
      newscn = insertSection(me, mrel, ms);
      if (!newscn) {
        ELFU_WARN("mReladd: Could not insert section %s (type %d), skipping.\n",
                  elfu_mScnName(mrel, ms),
                  ms->shdr.sh_type);
      }
      break;
  }

  return 0;
}


int subScnAdd2(ElfuElf *mrel, ElfuScn *ms, void *aux1, void *aux2)
{
  (void)aux2;
  ElfuElf *me = (ElfuElf*)aux1;
  (void)me;

  switch(ms->shdr.sh_type) {
    case SHT_NULL: /* 0 */
    case SHT_PROGBITS: /* 1 */
      break;

    case SHT_SYMTAB: /* 2 */
    case SHT_DYNSYM: /* 11 */
      /* Merge with the existing table. Take care of string tables also. */

    case SHT_STRTAB: /* 3 */
      /* May have to be merged with the existing string table for
       * the symbol table. */

    case SHT_RELA: /* 4 */
    case SHT_REL: /* 9 */
      /* Possibly append this in memory to the section model
       * that it describes. */

    case SHT_NOBITS: /* 8 */
      /* Expand this to SHT_PROGBITS, then insert as such. */

    case SHT_HASH: /* 5 */
    case SHT_DYNAMIC: /* 6 */
    case SHT_SHLIB: /* 10 */
    case SHT_SYMTAB_SHNDX: /* 18 */

    /* Don't care about the next ones yet. I've never seen
     * them and they can be implemented when necessary. */
    case SHT_NOTE: /* 7 */
    case SHT_INIT_ARRAY: /* 14 */
    case SHT_FINI_ARRAY: /* 15 */
    case SHT_PREINIT_ARRAY: /* 16 */
    case SHT_GROUP: /* 17 */
    case SHT_NUM: /* 19 */
    default:
      ELFU_WARN("mReladd: Skipping section %s (type %d).\n",
                elfu_mScnName(mrel, ms),
                ms->shdr.sh_type);
  }

  return 0;
}


void elfu_mReladd(ElfuElf *me, ElfuElf *mrel)
{
  assert(me);
  assert(mrel);

  /* For each section in object file, guess how to insert it */
  elfu_mScnForall(mrel, subScnAdd1, me, NULL);

  /* Do relocations and other stuff */
  elfu_mScnForall(mrel, subScnAdd2, me, NULL);
}
