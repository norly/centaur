#include <assert.h>
#include <stdlib.h>
#include <libelf/libelf.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>



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
