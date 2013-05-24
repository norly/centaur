#include <assert.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>


size_t elfu_mScnIndex(ElfuElf *me, ElfuScn *ms)
{
  ElfuScn *ms2;
  size_t i = 1;

  assert(me);
  assert(ms);

  CIRCLEQ_FOREACH(ms2, &me->scnList, elem) {
    if (ms2 == ms) {
      return i;
    }

    i++;
  }

  /* Section is not in ELF model. This means the calling code is broken. */
  assert(0);
}


/* NULL section is not counted! */
size_t elfu_mCountScns(ElfuElf *me)
{
  ElfuScn *ms;
  size_t i = 0;

  assert(me);

  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    i++;
  }

  return i;
}


size_t elfu_mCountPhdrs(ElfuElf *me)
{
  ElfuPhdr *mp;
  size_t i = 0;

  assert(me);

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    i++;
  }

  return i;
}
