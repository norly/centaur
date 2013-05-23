#include <libelf/libelf.h>
#include <libelf/gelf.h>

#include <libelfu/libelfu.h>


int elfu_ePhdrContainsScn(GElf_Phdr *phdr, GElf_Shdr *shdr)
{
  size_t secStart = shdr->sh_offset;
  size_t secEnd   = shdr->sh_offset + shdr->sh_size;
  size_t segStart = phdr->p_offset;
  size_t segEnd   = phdr->p_offset + phdr->p_memsz;

  if (secStart < segStart || secEnd > segEnd) {
    return 0;
  }

  return 1;
}
