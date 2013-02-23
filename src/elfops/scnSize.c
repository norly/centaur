#include <assert.h>
#include <sys/types.h>
#include <gelf.h>
#include <libelfu/libelfu.h>



size_t elfu_scnSizeFile(const GElf_Shdr *shdr)
{
  assert(shdr);

  return shdr->sh_type == SHT_NOBITS ? 0 : shdr->sh_size;
}
