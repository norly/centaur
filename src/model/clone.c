#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libelfu/libelfu.h>

ElfuScn* elfu_mCloneScn(ElfuScn *ms)
{
  ElfuScn *newscn;

  assert(ms);

  newscn = malloc(sizeof(ElfuScn));
  if (!newscn) {
    ELFU_WARN("elfu_nCloneScn: Could not allocate memory for new ElfuScn.\n");
    return NULL;
  }



  newscn->shdr = ms->shdr;
  newscn->data = ms->data;
  if (ms->data.d_buf) {
    void *newbuf = malloc(ms->data.d_size);
    if (!newbuf) {
      ELFU_WARN("elfu_nCloneScn: Could not allocate memory for new data buffer.\n");
      free(newscn);
      return NULL;
    }

    memcpy(newbuf, ms->data.d_buf, ms->data.d_size);
    newscn->data.d_buf = newbuf;
  }

  return newscn;
}
