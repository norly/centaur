#ifndef __LIBELFU_FIXUP_H__
#define __LIBELFU_FIXUP_H__

#include <libelf.h>
#include <gelf.h>

void elfu_fixupPhdrSelfRef(Elf *e);

#endif
