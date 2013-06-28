#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>


/* Meta-functions */

void* elfu_mScnForall(ElfuElf *me, SectionHandlerFunc f, void *aux1, void *aux2)
{
  ElfuPhdr *mp;
  ElfuScn *ms;

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }

    CIRCLEQ_FOREACH(ms, &mp->childScnList, elemChildScn) {
      void *rv = f(me, ms, aux1, aux2);

      if (rv) {
        return rv;
      }
    }
  }

  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elemChildScn) {
    void *rv = f(me, ms, aux1, aux2);

    if (rv) {
      return rv;
    }
  }

  return NULL;
}




/* Counting */

static void* subCounter(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  size_t *i = (size_t*)aux1;
  ElfuScn *otherScn = (ElfuScn*)aux2;

  if (ms == otherScn) {
    return ms;
  }

  *i += 1;

  /* Continue */
  return NULL;
}


size_t elfu_mScnCount(ElfuElf *me)
{
  /* NULL section *is not* counted */
  size_t i = 0;

  assert(me);

  elfu_mScnForall(me, subCounter, &i, NULL);

  return i;
}


/* Returns the index a section would have in the flattened ELF */
size_t elfu_mScnIndex(ElfuElf *me, ElfuScn *ms)
{
  /* NULL section *is* counted */
  size_t i = 1;

  assert(me);
  assert(ms);

  elfu_mScnForall(me, subCounter, &i, ms);

  /* If this assertion is broken then ms is not a section in me. */
  assert(i <= elfu_mScnCount(me));
  return i;
}


static void* subOldscn(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  ElfuScn *otherScn = (ElfuScn*)aux1;
  (void)aux2;

  if (ms->oldptr == otherScn) {
    return ms;
  }

  /* Continue */
  return NULL;
}

/* Returns the section with oldscn == oldscn */
ElfuScn* elfu_mScnByOldscn(ElfuElf *me, ElfuScn *oldscn)
{
  assert(me);
  assert(oldscn);

  return elfu_mScnForall(me, subOldscn, oldscn, NULL);
}




/* Convenience */

char* elfu_mScnName(ElfuElf *me, ElfuScn *ms)
{
  assert(me);
  assert(ms);

  if (!me->shstrtab) {
    return NULL;
  }

  if (!me->shstrtab->databuf) {
    return NULL;
  }

  return &me->shstrtab->databuf[ms->shdr.sh_name];
}


static void* subScnsToArray(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  ElfuScn **arr = (ElfuScn**)aux1;
  size_t *i = (size_t*)aux2;

  arr[(*i)] = ms;
  *i += 1;

  /* Continue */
  return NULL;
}

static int cmpScnOffs(const void *ms1, const void *ms2)
{
  ElfuScn *s1;
  ElfuScn *s2;

  assert(ms1);
  assert(ms2);

  s1 = *(ElfuScn**)ms1;
  s2 = *(ElfuScn**)ms2;

  assert(s1);
  assert(s2);


  if (s1->shdr.sh_offset < s2->shdr.sh_offset) {
    return -1;
  } else if (s1->shdr.sh_offset == s2->shdr.sh_offset) {
    return 0;
  } else /* if (s1->shdr.sh_offset > s2->shdr.sh_offset) */ {
    return 1;
  }
}

ElfuScn** elfu_mScnSortedByOffset(ElfuElf *me, size_t *count)
{
  size_t numSecs;
  ElfuScn **sortedSecs;
  size_t i;

  assert(me);

  /* Sort sections by offset in file */
  numSecs = elfu_mScnCount(me);
  sortedSecs = malloc(numSecs * sizeof(*sortedSecs));
  if (!sortedSecs) {
    ELFU_WARN("elfu_mScnSortedByOffset: Failed to allocate memory.\n");
    return NULL;
  }

  i = 0;
  elfu_mScnForall(me, subScnsToArray, sortedSecs, &i);
  assert(i == numSecs);

  qsort(sortedSecs, numSecs, sizeof(*sortedSecs), cmpScnOffs);

  *count = numSecs;

  return sortedSecs;
}



int elfu_mScnAppendData(ElfuScn *ms, void *buf, size_t len)
{
  char *newbuf;

  assert(ms);
  assert(ms->shdr.sh_type != SHT_NOBITS);
  assert(ms->databuf);

  newbuf = realloc(ms->databuf, ms->shdr.sh_size + len);
  if (!newbuf) {
    ELFU_WARN("elfu_mScnAppendData: malloc() failed for newbuf.\n");
    return -1;
  }

  ms->databuf = newbuf;
  memcpy(newbuf + ms->shdr.sh_size, buf, len);
  ms->shdr.sh_size += len;
  assert(ms->shdr.sh_size == ms->shdr.sh_size);

  return 0;
}



/*
 * Allocation, destruction
 */

ElfuScn* elfu_mScnAlloc()
{
  ElfuScn *ms;

  ms = malloc(sizeof(ElfuScn));
  if (!ms) {
    ELFU_WARN("mScnCreate: malloc() failed for ElfuScn.\n");
    return NULL;
  }

  memset(ms, 0, sizeof(*ms));

  CIRCLEQ_INIT(&ms->symtab.syms);
  CIRCLEQ_INIT(&ms->reltab.rels);

  return ms;
}

void elfu_mScnDestroy(ElfuScn* ms)
{
  assert(ms);

  if (!CIRCLEQ_EMPTY(&ms->symtab.syms)) {
    ElfuSym *nextsym;

    nextsym = CIRCLEQ_FIRST(&ms->symtab.syms);
    while ((void*)nextsym != (void*)&ms->symtab.syms) {
      ElfuSym *cursym = nextsym;
      nextsym = CIRCLEQ_NEXT(cursym, elem);
      CIRCLEQ_REMOVE(&ms->symtab.syms, cursym, elem);
      free(cursym);
    }
  }

  if (!CIRCLEQ_EMPTY(&ms->reltab.rels)) {
    ElfuRel *nextrel;

    nextrel = CIRCLEQ_FIRST(&ms->reltab.rels);
    while ((void*)nextrel != (void*)&ms->reltab.rels) {
      ElfuRel *currel = nextrel;
      nextrel = CIRCLEQ_NEXT(currel, elem);
      CIRCLEQ_REMOVE(&ms->reltab.rels, currel, elem);
      free(currel);
    }
  }

  if (ms->databuf) {
    free(ms->databuf);
  }

  free(ms);
}
