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
#include <string.h>
#include <libelfu/libelfu.h>


/* Meta-functions */

void* elfu_mPhdrForall(ElfuElf *me, PhdrHandlerFunc f, void *aux1, void *aux2)
{
  ElfuPhdr *mp;

  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    ElfuPhdr *mp2;
    void *rv = f(me, mp, aux1, aux2);
    if (rv) {
      return rv;
    }

    CIRCLEQ_FOREACH(mp2, &mp->childPhdrList, elemChildPhdr) {
      void *rv = f(me, mp2, aux1, aux2);
      if (rv) {
        return rv;
      }
    }
  }

  return NULL;
}




/* Counting */

static void* subCounter(ElfuElf *me, ElfuPhdr *mp, void *aux1, void *aux2)
{
  size_t *i = (size_t*)aux1;
  (void)aux2;

  *i += 1;

  /* Continue */
  return NULL;
}

size_t elfu_mPhdrCount(ElfuElf *me)
{
  size_t i = 0;

  assert(me);

  elfu_mPhdrForall(me, subCounter, &i, NULL);

  return i;
}




/* Finding by exact address/offset */

static void* subFindLoadByAddr(ElfuElf *me, ElfuPhdr *mp, void *aux1, void *aux2)
{
  GElf_Addr addr = *(GElf_Addr*)aux1;
  (void)aux2;

  if (mp->phdr.p_type == PT_LOAD
      && FULLY_OVERLAPPING(mp->phdr.p_vaddr, mp->phdr.p_memsz, addr, 1)) {
    return mp;
  }

  /* Continue */
  return NULL;
}

ElfuPhdr* elfu_mPhdrByAddr(ElfuElf *me, GElf_Addr addr)
{
  return elfu_mPhdrForall(me, subFindLoadByAddr, &addr, NULL);
}


static void* subFindLoadByOffset(ElfuElf *me, ElfuPhdr *mp, void *aux1, void *aux2)
{
  GElf_Off offset = *(GElf_Off*)aux1;
  (void)aux2;

  if (mp->phdr.p_type == PT_LOAD
      && FULLY_OVERLAPPING(mp->phdr.p_offset, mp->phdr.p_filesz, offset, 1)) {
    return mp;
  }

  /* Continue */
  return NULL;
}

ElfuPhdr* elfu_mPhdrByOffset(ElfuElf *me, GElf_Off offset)
{
  return elfu_mPhdrForall(me, subFindLoadByOffset, &offset, NULL);
}




/* Find lowest/highest address/offset */

void elfu_mPhdrLoadLowestHighest(ElfuElf *me,
                                 ElfuPhdr **lowestAddr, ElfuPhdr **highestAddr,
                                 ElfuPhdr **lowestOffs, ElfuPhdr **highestOffsEnd)
{
  ElfuPhdr *mp;

  assert(me);
  assert(lowestAddr);
  assert(highestAddr);
  assert(lowestOffs);
  assert(highestOffsEnd);

  *lowestAddr = NULL;
  *highestAddr = NULL;
  *lowestOffs = NULL;
  *highestOffsEnd = NULL;

  /* Find first and last LOAD PHDRs.
   * Don't compare p_memsz - segments don't overlap in memory. */
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    if (mp->phdr.p_type != PT_LOAD) {
      continue;
    }
    if (!*lowestAddr || mp->phdr.p_vaddr < (*lowestAddr)->phdr.p_vaddr) {
      *lowestAddr = mp;
    }
    if (!*highestAddr || mp->phdr.p_vaddr > (*highestAddr)->phdr.p_vaddr) {
      *highestAddr = mp;
    }
    if (!*lowestOffs || mp->phdr.p_offset < (*lowestOffs)->phdr.p_offset) {
      *lowestOffs = mp;
    }
    if (!*highestOffsEnd
        || (OFFS_END(mp->phdr.p_offset,
                     mp->phdr.p_filesz)
            > OFFS_END((*highestOffsEnd)->phdr.p_offset,
                       (*highestOffsEnd)->phdr.p_filesz))) {
      *highestOffsEnd = mp;
    }
  }
}




/* Layout update */

void elfu_mPhdrUpdateChildOffsets(ElfuPhdr *mp)
{
  ElfuScn *ms;
  ElfuPhdr *mpc;

  assert(mp);
  assert(mp->phdr.p_type == PT_LOAD);

  CIRCLEQ_FOREACH(mpc, &mp->childPhdrList, elemChildPhdr) {
    mpc->phdr.p_offset = mp->phdr.p_offset + (mpc->phdr.p_vaddr - mp->phdr.p_vaddr);
  }

  CIRCLEQ_FOREACH(ms, &mp->childScnList, elemChildScn) {
    ms->shdr.sh_offset = mp->phdr.p_offset + (ms->shdr.sh_addr - mp->phdr.p_vaddr);
  }
}



/*
 * Allocation, destruction
 */

ElfuPhdr* elfu_mPhdrAlloc()
{
  ElfuPhdr *mp;

  mp = malloc(sizeof(ElfuPhdr));
  if (!mp) {
    ELFU_WARN("mPhdrAlloc: malloc() failed for ElfuPhdr.\n");
    return NULL;
  }

  memset(mp, 0, sizeof(*mp));

  CIRCLEQ_INIT(&mp->childScnList);
  CIRCLEQ_INIT(&mp->childPhdrList);

  return mp;
}

void elfu_mPhdrDestroy(ElfuPhdr* mp)
{
  assert(mp);

  if (!CIRCLEQ_EMPTY(&mp->childPhdrList)) {
    ElfuPhdr *nextmp;

    nextmp = CIRCLEQ_FIRST(&mp->childPhdrList);
    while ((void*)nextmp != (void*)&mp->childPhdrList) {
      ElfuPhdr *curmp = nextmp;
      nextmp = CIRCLEQ_NEXT(curmp, elemChildPhdr);
      CIRCLEQ_REMOVE(&mp->childPhdrList, curmp, elemChildPhdr);
      elfu_mPhdrDestroy(curmp);
    }
  }

  if (!CIRCLEQ_EMPTY(&mp->childScnList)) {
    ElfuScn *nextms;

    nextms = CIRCLEQ_FIRST(&mp->childScnList);
    while ((void*)nextms != (void*)&mp->childScnList) {
      ElfuScn *curms = nextms;
      nextms = CIRCLEQ_NEXT(curms, elemChildScn);
      CIRCLEQ_REMOVE(&mp->childScnList, curms, elemChildScn);
      elfu_mScnDestroy(curms);
    }
  }

  free(mp);
}
