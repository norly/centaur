#ifndef __LIBELFU_ANALYSIS_H_
#define __LIBELFU_ANALYSIS_H_

#include <libelf.h>
#include <gelf.h>

#include <libelfu/types.h>

int elfu_segmentContainsSection(GElf_Phdr *phdr, GElf_Shdr *shdr);

#endif
