#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


/* Meta-functions */

void* elfu_mPhdrForall(ElfuElf *me, PhdrHandlerFunc f, void *aux1, void *aux2)
{
  ElfuPhdr *mp;

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    ElfuPhdr *mp2;
    void *rv = f(me, mp, aux1, aux2);
    if (rv) {
      return rv;
    }

    CIRCLEQ_FOREACH(mp2, &mp->childPhdrList, elemChildPhdr) {
      void *rv = f(me, mp2, aux1, aux2);
      if (rv) {
        return rv;
      }
    }
  }

  return NULL;
}




/* Counting */

static void* subCounter(ElfuElf *me, ElfuPhdr *mp, void *aux1, void *aux2)
{
  size_t *i = (size_t*)aux1;
  (void)aux2;

  *i += 1;

  /* Continue */
  return NULL;
}


size_t elfu_mPhdrCount(ElfuElf *me)
{
  size_t i = 0;

  assert(me);

  elfu_mPhdrForall(me, subCounter, &i, NULL);

  return i;
}



/* Layout update */

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
    CIRCLEQ_REMOVE(&mp->childPhdrList, mp2, elem);
    elfu_mPhdrDestroy(mp2);
  }

  CIRCLEQ_FOREACH(ms, &mp->childScnList, elem) {
    CIRCLEQ_REMOVE(&mp->childScnList, ms, elem);
    elfu_mScnDestroy(ms);
  }

  free(mp);
}
