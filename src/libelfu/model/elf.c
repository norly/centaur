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
  assert(me);

  if (!CIRCLEQ_EMPTY(&me->phdrList)) {
    ElfuPhdr *nextmp;

    nextmp = CIRCLEQ_FIRST(&me->phdrList);
    while ((void*)nextmp != (void*)&me->phdrList) {
      ElfuPhdr *curmp = nextmp;
      nextmp = CIRCLEQ_NEXT(curmp, elem);
      CIRCLEQ_REMOVE(&me->phdrList, curmp, elem);
      elfu_mPhdrDestroy(curmp);
    }
  }

  if (!CIRCLEQ_EMPTY(&me->orphanScnList)) {
    ElfuScn *nextms;

    nextms = CIRCLEQ_FIRST(&me->orphanScnList);
    while ((void*)nextms != (void*)&me->orphanScnList) {
      ElfuScn *curms = nextms;
      nextms = CIRCLEQ_NEXT(curms, elemChildScn);
      CIRCLEQ_REMOVE(&me->orphanScnList, curms, elemChildScn);
      elfu_mScnDestroy(curms);
    }
  }

  free(me);
}
