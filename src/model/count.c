#include <assert.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>



/* NULL section is not counted! */
size_t elfu_countSections(ElfuElf *me)
{
  ElfuScn *ms;
  size_t i = 0;

  assert(me);

  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    i++;
  }

  return i;
}


size_t elfu_countPHDRs(ElfuElf *me)
{
  ElfuPhdr *mp;
  size_t i = 0;

  assert(me);

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    i++;
  }

  return i;
}
