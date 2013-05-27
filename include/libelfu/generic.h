#ifndef __LIBELFU_GENERIC_H__
#define __LIBELFU_GENERIC_H__

#include <libelf/gelf.h>

#define OFFS_END(off, sz) ((off) + (sz))

#define OVERLAPPING(off1, sz1, off2, sz2) \
  (!((off1) == (off2) && ((sz1 == 0) || (sz2 == 0))) \
   && (((off1) <= (off2) && (off2) < OFFS_END((off1), (sz1))) \
       || ((off2) <= (off1) && (off1) < OFFS_END((off2), (sz2)))) \
  )

#define FULLY_OVERLAPPING(off1, sz1, off2, sz2) \
  (((off1) <= (off2) && OFFS_END((off2), (sz2)) <= OFFS_END((off1), (sz1))) \
   || ((off2) <= (off1) && OFFS_END((off1), (sz1)) <= OFFS_END((off2), (sz2))))



#define SCNFILESIZE(shdr) ((shdr)->sh_type == SHT_NOBITS ? 0 : (shdr)->sh_size)


int elfu_gPhdrContainsScn(GElf_Phdr *phdr, GElf_Shdr *shdr);


#endif
