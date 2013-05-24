#ifndef __LIBELFU_GENERIC_H__
#define __LIBELFU_GENERIC_H__

#include <libelf/gelf.h>


size_t elfu_gScnSizeFile(const GElf_Shdr *shdr);

int elfu_gPhdrContainsScn(GElf_Phdr *phdr, GElf_Shdr *shdr);

#endif
