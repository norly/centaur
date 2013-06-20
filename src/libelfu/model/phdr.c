#include <assert.h>
#include <stdlib.h>
#include <string.h>
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



/*
 * Allocation, destruction
 */

ElfuPhdr* elfu_mPhdrAlloc()
{
  ElfuPhdr *mp;

  mp = malloc(sizeof(ElfuPhdr));
  if (!mp) {
    ELFU_WARN("mPhdrAlloc: malloc() failed for ElfuPhdr.\n");
    return NULL;
  }

  memset(mp, 0, sizeof(*mp));

  CIRCLEQ_INIT(&mp->childScnList);
  CIRCLEQ_INIT(&mp->childPhdrList);

  return mp;
}

void elfu_mPhdrDestroy(ElfuPhdr* mp)
{
  ElfuPhdr *mp2;
  ElfuScn *ms;

  assert(mp);

  CIRCLEQ_FOREACH(mp2, &mp->childPhdrList, elem) {
    // TODO ?
  }

  CIRCLEQ_FOREACH(ms, &mp->childScnList, elem) {
    CIRCLEQ_REMOVE(&mp->childScnList, ms, elem);
    elfu_mScnDestroy(ms);
  }

  free(mp);
}
