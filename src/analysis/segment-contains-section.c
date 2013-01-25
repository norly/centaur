#include <libelf.h>
#include <gelf.h>

#include <libelfu/libelfu.h>


ELFU_BOOL elfu_segmentContainsSection(GElf_Phdr *phdr, Elf_Scn *scn)
{
  GElf_Shdr shdr;


  if (gelf_getshdr(scn, &shdr) != &shdr) {
    return ELFU_ERROR;
  }

  size_t secStart = shdr.sh_offset;
  size_t secEnd   = shdr.sh_offset + shdr.sh_size;
  size_t segStart = phdr->p_offset;
  size_t segEnd   = phdr->p_offset + phdr->p_memsz;

  if (secStart < segStart || secEnd > segEnd) {
    return ELFU_FALSE;
  }

  return ELFU_TRUE;
}
