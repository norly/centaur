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

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libelfu/libelfu.h>


int elfu_mCheck(ElfuElf *me)
{
  size_t numSecs;
  ElfuScn **sortedSecs;
  size_t i;

  sortedSecs = elfu_mScnSortedByOffset(me, &numSecs);
  if (!sortedSecs) {
    return -1;
  }


  /* Check for overlapping sections */
  for (i = 0; i < numSecs - 1; i++) {
    if (sortedSecs[i]->shdr.sh_offset + SCNFILESIZE(&sortedSecs[i]->shdr)
        > sortedSecs[i+1]->shdr.sh_offset) {
      ELFU_WARN("elfu_check: Found overlapping sections: %s and %s.\n",
                elfu_mScnName(me, sortedSecs[i]),
                elfu_mScnName(me, sortedSecs[i+1]));
    }
  }


  /* Check for sections overlapping with EHDR */
  for (i = 0; i < numSecs; i++) {
    if (sortedSecs[i]->shdr.sh_offset < me->ehdr.e_ehsize) {
      ELFU_WARN("elfu_check: Found section overlapping with EHDR: %s.\n",
                elfu_mScnName(me, sortedSecs[i]));
    }
  }


  /* Check for sections overlapping with PHDRs */
  for (i = 0; i < numSecs; i++) {
    if (OVERLAPPING(sortedSecs[i]->shdr.sh_offset,
                    SCNFILESIZE(&sortedSecs[i]->shdr),
                    me->ehdr.e_phoff,
                    me->ehdr.e_phentsize * me->ehdr.e_phnum)) {
      ELFU_WARN("elfu_check: Found section overlapping with PHDRs: %s.\n",
                elfu_mScnName(me, sortedSecs[i]));
    }
  }


  free(sortedSecs);

  return 0;
}
