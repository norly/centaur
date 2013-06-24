#include <assert.h>
#include <stdlib.h>
#include <libelfu/libelfu.h>


static int cmpPhdrType(const void *v1, const void *v2)
{
  const GElf_Phdr *p1 = (const GElf_Phdr*)v1;
  const GElf_Phdr *p2 = (const GElf_Phdr*)v2;

  /* These entries go first */
  if (p1->p_type == PT_PHDR) {
    return -1;
  } else if (p2->p_type == PT_PHDR) {
    return 1;
  } else if (p1->p_type == PT_INTERP) {
    return -1;
  } else if (p2->p_type == PT_INTERP) {
    return 1;
  }

  /* These entries go last */
  if (p1->p_type == PT_GNU_RELRO) {
    return 1;
  } else if (p2->p_type == PT_GNU_RELRO) {
    return -1;
  } else if (p1->p_type == PT_GNU_STACK) {
    return 1;
  } else if (p2->p_type == PT_GNU_STACK) {
    return -1;
  } else if (p1->p_type == PT_GNU_EH_FRAME) {
    return 1;
  } else if (p2->p_type == PT_GNU_EH_FRAME) {
    return -1;
  } else if (p1->p_type == PT_NOTE) {
    return 1;
  } else if (p2->p_type == PT_NOTE) {
    return -1;
  } else if (p1->p_type == PT_DYNAMIC) {
    return 1;
  } else if (p2->p_type == PT_DYNAMIC) {
    return -1;
  }

  /* Sort the rest by vaddr. */
  if (p1->p_vaddr < p2->p_vaddr) {
    return -1;
  } else if (p1->p_vaddr > p2->p_vaddr) {
    return 1;
  }

  /* Everything else is equal */
  return 0;
}



/* The ELF specification wants PHDR and INTERP headers to come first.
 * Typical GNU programs also contain DYNAMIC, NOTE, and GNU_* after
 * the LOAD segments.
 * Reorder them to comply with the spec. Otherwise the dynamic linker
 * will calculate the base address incorrectly and crash.
 *
 * Both Linux and glibc's ld.so are unhelpful here:
 * Linux calculates the PH table address as
 *     phdrs = (base + e_phoff)
 * and passes that to ld.so.
 * ld.so 'recovers' the base address as
 *     base = (phdrs - PT_PHDR.p_vaddr)
 *
 * Behold those who try to move the PHDR table away from the first
 * page of memory, where it is stored at the address calculated by
 * Linux. Things will crash. Badly.
 *
 * Unfortunately the ELF spec itself states that the PHT is in the
 * first page of memory. Not much we can do here.
 */
void elfu_eReorderPhdrs(Elf *e)
{
  size_t i, numPhdr;

  assert(e);

  assert(!elf_getphdrnum(e, &numPhdr));

  GElf_Phdr phdrs[numPhdr];

  for (i = 0; i < numPhdr; i++) {
    GElf_Phdr phdr;

    if (gelf_getphdr(e, i, &phdr) != &phdr) {
      ELFU_WARN("gelf_getphdr() failed for #%d: %s\n", i, elf_errmsg(-1));
      return;
    }

    phdrs[i] = phdr;
  }

  qsort(phdrs, numPhdr, sizeof(*phdrs), cmpPhdrType);

  for (i = 0; i < numPhdr; i++) {
    assert(gelf_update_phdr(e, i, &phdrs[i]));
  }
}
