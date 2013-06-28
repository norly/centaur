#ifndef __ELFHANDLE_H__
#define __ELFHANDLE_H__

#include <libelf.h>

/*!
 * A simple pair of a file descriptor and a libelf handle,
 * used to simplify elfucli.
 */
typedef struct {
  int fd;   /*!< File handle */
  Elf *e;   /*!< libelf handle */
} ELFHandles;


void openElf(ELFHandles *h, char *fn, Elf_Cmd elfmode);
void closeElf(ELFHandles *h);

#endif
