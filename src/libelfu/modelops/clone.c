#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>

ElfuScn* elfu_mCloneScn(ElfuScn *ms)
{
  ElfuScn *newscn;

  assert(ms);

  newscn = elfu_mScnAlloc();
  if (!newscn) {
    ELFU_WARN("elfu_nCloneScn: Could not allocate memory for new ElfuScn.\n");
    return NULL;
  }

  newscn->shdr = ms->shdr;

  if (ms->databuf) {
    void *newbuf = malloc(ms->shdr.sh_size);
    if (!newbuf) {
      ELFU_WARN("elfu_nCloneScn: Could not allocate memory for new data buffer.\n");
      free(newscn);
      return NULL;
    }

    memcpy(newbuf, ms->databuf, ms->shdr.sh_size);
    newscn->databuf = newbuf;
  }

  newscn->oldptr = ms;

  return newscn;
}
