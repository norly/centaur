#include <assert.h>
#include <stdlib.h>

#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>



char* elfu_modelScnName(ElfuElf *me, ElfuScn *ms)
{
  assert(me);
  assert(ms);

  if (!me->shstrtab) {
    return NULL;
  }

  if (CIRCLEQ_EMPTY(&me->shstrtab->dataList)) {
    return NULL;
  }

  /* Don't take multiple data parts into account. */
  ElfuData *md = me->shstrtab->dataList.cqh_first;
  return &((char*)md->data.d_buf)[ms->shdr.sh_name];
}
