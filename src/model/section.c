#include <assert.h>
#include <stdlib.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>


/* Meta-functions */

int elfu_mScnForall(ElfuElf *me, SectionHandlerFunc f, void *aux1, void *aux2)
{
  ElfuPhdr *mp;
  ElfuScn *ms;

  // TODO: Sort PHDRs by offset before interating

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }

    CIRCLEQ_FOREACH(ms, &mp->childScnList, elemChildScn) {
      int rv = f(me, ms, aux1, aux2);

      if (rv) {
        return rv;
      }
    }
  }

  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elemChildScn) {
    int rv = f(me, ms, aux1, aux2);

    if (rv) {
      return rv;
    }
  }

  return 0;
}




/* Counting */

static int subCounter(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  size_t *i = (size_t*)aux1;
  ElfuScn *otherScn = (ElfuScn*)aux2;

  if (ms == otherScn) {
    return 1;
  }

  *i += 1;

  return 0;
}


size_t elfu_mScnCount(ElfuElf *me)
{
  /* NULL section *is not* counted */
  size_t i = 0;

  assert(me);

  elfu_mScnForall(me, subCounter, &i, NULL);

  return i;
}


/* Returns the section index equivalent to the model flattened to ELF */
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




/* Convenience */

char* elfu_mScnName(ElfuElf *me, ElfuScn *ms)
{
  assert(me);
  assert(ms);

  if (!me->shstrtab) {
    return NULL;
  }

  if (!me->shstrtab->data.d_buf) {
    return NULL;
  }

  return &((char*)me->shstrtab->data.d_buf)[ms->shdr.sh_name];
}


static int subScnsToArray(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  ElfuScn **arr = (ElfuScn**)aux1;
  size_t *i = (size_t*)aux2;

  arr[(*i)] = ms;
  *i += 1;

  return 0;
}

static int cmpScnOffs(const void *ms1, const void *ms2)
{
  assert(ms1);
  assert(ms2);

  ElfuScn *s1 = *(ElfuScn**)ms1;
  ElfuScn *s2 = *(ElfuScn**)ms2;

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
  assert(me);

  size_t numSecs;
  ElfuScn **sortedSecs;
  size_t i;

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
