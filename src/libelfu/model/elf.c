#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


/*
 * Allocation, destruction
 */

ElfuElf* elfu_mElfAlloc()
{
  ElfuElf *me;

  me = malloc(sizeof(ElfuElf));
  if (!me) {
    ELFU_WARN("mElfAlloc: malloc() failed for ElfuElf.\n");
    return NULL;
  }

  memset(me, 0, sizeof(*me));

  CIRCLEQ_INIT(&me->phdrList);
  CIRCLEQ_INIT(&me->orphanScnList);

  return me;
}


void elfu_mElfDestroy(ElfuElf* me)
{
  ElfuPhdr *mp;
  ElfuScn *ms;

  assert(me);

  CIRCLEQ_INIT(&me->phdrList);
  CIRCLEQ_INIT(&me->orphanScnList);

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    CIRCLEQ_REMOVE(&me->phdrList, mp, elem);
    elfu_mPhdrDestroy(mp);
  }

  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elem) {
    CIRCLEQ_REMOVE(&me->orphanScnList, ms, elem);
    elfu_mScnDestroy(ms);
  }

  free(me);
}
