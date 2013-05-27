#include <assert.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>


/*
 * Returns the first section beginning in the segment.
 *
 * Since sections have to be in the file in mapping order to load them
 * as a continuous segment, we only have to search by start offset.
 */
ElfuScn* elfu_mScnFirstInSegment(ElfuElf *me, ElfuPhdr *mp)
{
  ElfuScn *ms;

  assert(me);
  assert(mp);

  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if ((ms->shdr.sh_offset >= mp->phdr.p_offset)
        && (ms->shdr.sh_offset < mp->phdr.p_offset + mp->phdr.p_filesz)) {
      return ms;
    }
  }

  return NULL;
}



/*
 * Returns the last section ending in the segment.
 *
 * Since sections have to be in the file in mapping order to load them
 * as a continuous segment, we only have to search by end offset.
 */
ElfuScn* elfu_mScnLastInSegment(ElfuElf *me, ElfuPhdr *mp)
{
  ElfuScn *last = NULL;
  ElfuScn *ms;

  assert(me);
  assert(mp);

  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    /* Get section size on disk - for NOBITS sections that is 0 bytes. */
    size_t size = SCNFILESIZE(&ms->shdr);

    if (((ms->shdr.sh_offset + size >= mp->phdr.p_offset)
         && (ms->shdr.sh_offset + size <= mp->phdr.p_offset + mp->phdr.p_filesz))) {
      last = ms;
    }
  }

  return last;
}
