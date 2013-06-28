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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libelf.h>

#include "elfhandle.h"


void openElf(ELFHandles *h, char *fn, Elf_Cmd elfmode)
{
  int openflags = 0;
  int openmode = 0;

  h->e = NULL;

  switch(elfmode) {
    case ELF_C_READ:
      openflags = O_RDONLY;
      break;
    case ELF_C_WRITE:
      openflags = O_WRONLY | O_CREAT;
      openmode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
      break;
    case ELF_C_RDWR:
      openflags = O_RDWR;
      break;
    default:
      return;
  }


  h->fd = open(fn, openflags, openmode);
  if (h->fd < 0) {
    fprintf(stderr, "Cannot open %s: %s\n", fn, strerror(errno));
    return;
  }

  h->e = elf_begin(h->fd, elfmode, NULL);
  if (!h->e) {
    fprintf(stderr, "elf_begin() failed on %s: %s\n", fn, elf_errmsg(-1));
    goto ERR;
  }

  if (elfmode == ELF_C_READ || elfmode == ELF_C_RDWR) {
    if (elf_kind(h->e) != ELF_K_ELF) {
      fprintf(stderr, "Not an ELF object: %s", fn);
      goto ERR;
    }
  }

  return;

ERR:
  if (h->e) {
    elf_end(h->e);
    h->e = NULL;
  }
  close(h->fd);
}


void closeElf(ELFHandles *h)
{
  if (h->e) {
    elf_end(h->e);
    close(h->fd);
    h->e = NULL;
  }
}
