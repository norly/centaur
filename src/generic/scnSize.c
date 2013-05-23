#include <assert.h>
#include <sys/types.h>
#include <libelf/gelf.h>
#include <libelfu/libelfu.h>



size_t elfu_gScnSizeFile(const GElf_Shdr *shdr)
{
  assert(shdr);

  return shdr->sh_type == SHT_NOBITS ? 0 : shdr->sh_size;
}
