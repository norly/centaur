#include <assert.h>
#include <libelfu/libelfu.h>


size_t elfu_mPhdrCount(ElfuElf *me)
{
  ElfuPhdr *mp;
  size_t i = 0;

  assert(me);

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    i++;
  }

  return i;
}
