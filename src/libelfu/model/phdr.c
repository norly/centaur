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



void elfu_mPhdrUpdateChildOffsets(ElfuPhdr *mp)
{
  ElfuScn *ms;
  ElfuPhdr *mpc;

  assert(mp);
  assert(mp->phdr.p_type == PT_LOAD);

  CIRCLEQ_FOREACH(mpc, &mp->childPhdrList, elemChildPhdr) {
    mpc->phdr.p_offset = mp->phdr.p_offset + (mpc->phdr.p_vaddr - mp->phdr.p_vaddr);
  }

  CIRCLEQ_FOREACH(ms, &mp->childScnList, elemChildScn) {
    ms->shdr.sh_offset = mp->phdr.p_offset + (ms->shdr.sh_addr - mp->phdr.p_vaddr);
  }
}
