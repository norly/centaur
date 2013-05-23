#ifndef __ELFHANDLE_H__
#define __ELFHANDLE_H__

#include <libelf/libelf.h>

typedef struct {
  int fd;
  Elf *e;
} ELFHandles;


void openElf(ELFHandles *h, char *fn, Elf_Cmd elfmode);
void closeElf(ELFHandles *h);

#endif
