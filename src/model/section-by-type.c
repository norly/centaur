#include <assert.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>


/*
 * Returns the first section with the given type.
 */
ElfuScn* elfu_mScnByType(ElfuElf *me, Elf32_Word type)
{
  ElfuScn *ms;

  assert(me);

  CIRCLEQ_FOREACH(ms, &me->scnList, elem) {
    if (ms->shdr.sh_type == type) {
      return ms;
    }
  }

  return NULL;
}
