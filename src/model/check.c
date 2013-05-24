#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>



static int isOverlapping(size_t off1, size_t sz1, size_t off2, size_t sz2)
{
  size_t end1 = off1 + sz1;
  size_t end2 = off2 + sz2;

  if (off2 >= off1 && off2 < end1) {
    return 1;
  } else if (off1 >= off2 && off1 < end2) {
    return 1;
  } else {
    return 0;
  }
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


int elfu_mCheck(ElfuElf *me)
{
  ElfuScn *ms;
  size_t numSecs;
  ElfuScn **sortedSecs;
  size_t i;

  /* Sort sections by offset in file */
  numSecs = elfu_mCountScns(me);
  sortedSecs = malloc(numSecs * sizeof(*sortedSecs));
  if (!sortedSecs) {
    ELFU_WARN("elfu_check: Failed to allocate memory.\n");
  }

  i = 0;
  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    sortedSecs[i] = ms;
    i++;
  }
  assert(i = numSecs);

  qsort(sortedSecs, numSecs, sizeof(*sortedSecs), cmpScnOffs);


  /* Check for overlapping sections */
  for (i = 0; i < numSecs - 1; i++) {
    if (sortedSecs[i]->shdr.sh_offset + elfu_gScnSizeFile(&sortedSecs[i]->shdr)
        > sortedSecs[i+1]->shdr.sh_offset) {
      ELFU_WARN("elfu_check: Found overlapping sections: %s and %s.\n",
                elfu_mScnName(me, sortedSecs[i]),
                elfu_mScnName(me, sortedSecs[i+1]));
    }
  }


  /* Check for sections overlapping with EHDR */
  for (i = 0; i < numSecs; i++) {
    if (sortedSecs[i]->shdr.sh_offset < me->ehdr.e_ehsize) {
      ELFU_WARN("elfu_check: Found section overlapping with EHDR: %s.\n",
                elfu_mScnName(me, sortedSecs[i]));
    }
  }


  /* Check for sections overlapping with PHDRs */
  for (i = 0; i < numSecs; i++) {
    if (isOverlapping(sortedSecs[i]->shdr.sh_offset,
                      elfu_gScnSizeFile(&sortedSecs[i]->shdr),
                      me->ehdr.e_phoff,
                      me->ehdr.e_phentsize * me->ehdr.e_phnum)) {
      ELFU_WARN("elfu_check: Found section overlapping with PHDRs: %s.\n",
                elfu_mScnName(me, sortedSecs[i]));
    }
  }

  free(sortedSecs);

  return 0;
}
