/* This file is part of centaur.
 *
 * centaur is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 2 as
 * published by the Free Software Foundation.

 * centaur is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with centaur.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBELFU_GENERIC_H__
#define __LIBELFU_GENERIC_H__

#include <gelf.h>


#ifndef MIN
  #define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
  #define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ROUNDUP
  #define ROUNDUP(x, align) ((x) + ((align) - ((x) % (align))) % (align))
#endif


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



#define PHDR_CONTAINS_SCN_IN_MEMORY(phdr, shdr) \
  (((phdr)->p_vaddr) <= ((shdr)->sh_addr) \
   && OFFS_END((shdr)->sh_addr, (shdr)->sh_size) <= OFFS_END((phdr)->p_vaddr, (phdr)->p_memsz))

#define PHDR_CONTAINS_SCN_IN_FILE(phdr, shdr) \
  (((phdr)->p_offset) <= ((shdr)->sh_offset) \
   && OFFS_END((shdr)->sh_offset, SCNFILESIZE(shdr)) <= OFFS_END((phdr)->p_offset, (phdr)->p_filesz))


#endif
