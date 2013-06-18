#include <libelfu/libelfu.h>
#include <string.h>

static void* subFindByAddr(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  GElf_Addr a = *(GElf_Addr*)aux1;

  if (OVERLAPPING(ms->shdr.sh_addr, ms->shdr.sh_size, a, 1)) {
    return ms;
  }

  /* Continue */
  return NULL;
}


void elfu_mDetour(ElfuElf *me, GElf_Addr from, GElf_Addr to)
{
  ElfuScn *ms;
  GElf_Word scnoffset;
  unsigned char detourcode[] = {0xe9, 0xfc, 0xff, 0xff, 0xff};

  ms = elfu_mScnForall(me, subFindByAddr, &from, NULL);

  if (!ms) {
    ELFU_WARN("mDetour: Cannot find address %x in any section.\n",
              (unsigned)from);
    return;
  }

  if (ms->shdr.sh_type != SHT_PROGBITS) {
    ELFU_WARN("mDetour: Cannot detour in non-PROGBITS section %s.\n",
              elfu_mScnName(me, ms));
    return;
  }

  scnoffset = from - ms->shdr.sh_addr;

  if (ms->shdr.sh_size - scnoffset < 5) {
    ELFU_WARN("mDetour: Not enough space to insert a detour.\n");
    return;
  }

  ELFU_DEBUG("mDetour: Detouring at address %x in section %s to %x.\n",
             (unsigned)from,
             elfu_mScnName(me, ms),
             (unsigned)to);

  *(Elf32_Word*)(detourcode + 1) = to - from - 5;
  memcpy((char*)ms->data.d_buf + scnoffset, detourcode, 5);
}
