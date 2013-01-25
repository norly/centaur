#ifndef __LIBELFU_ANALYSIS_H_
#define __LIBELFU_ANALYSIS_H_

#include <libelf.h>
#include <gelf.h>

#include <libelfu/types.h>

ELFU_BOOL elfu_segmentContainsSection(GElf_Phdr *phdr, Elf_Scn *scn);

#endif
