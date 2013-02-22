#include <sys/types.h>
#include <libelfu/libelfu.h>



size_t elfu_countSections(ElfuElf *me)
{
  ElfuScn *ms;
  size_t i = 0;

  for (ms = me->scnList.cqh_first;
        ms != (void *)&me->scnList;
        ms = ms->elem.cqe_next) {
    i++;
  }

  return i;
}


size_t elfu_countPHDRs(ElfuElf *me)
{
  ElfuPhdr *mp;
  size_t i = 0;

  for (mp = me->phdrList.cqh_first;
        mp != (void *)&me->phdrList;
        mp = mp->elem.cqe_next) {
    i++;
  }

  return i;
}
