/*!
 * @file types.h
 * @brief libelfu's Elfu* types.
 *
 * libelfu provides an abstraction of the data in ELF files
 * to more closely model the dependencies between them, and
 * thus to facilitate consistent editing.
 */

#ifndef __LIBELFU_TYPES_H__
#define __LIBELFU_TYPES_H__

#include <sys/queue.h>

#include <elf.h>
#include <gelf.h>


/*!
 * ELFU model of a symbol table entry.
 */
typedef struct ElfuSym {
  /*! Offset into linked string table */
  GElf_Word name;


  /*! Explicit value */
  GElf_Addr value;

  /*! Size of object referenced */
  GElf_Word size;


  /*! Binding type (GLOBAL, LOCAL, ...) */
  unsigned char bind;

  /*! Type (FUNC, SECTION, ...) */
  unsigned char type;

  /*! Always 0, currently meaningless (see ELF spec) */
  unsigned char other;


  /*! If non-null, the section referenced. */
  struct ElfuScn *scnptr;

  /*! If scnptr is invalid, then this is ABS, COMMON, or UNDEF. */
  int shndx;


  /*! Entry in a symbol table */
  CIRCLEQ_ENTRY(ElfuSym) elem;
} ElfuSym;



/*!
 * ELFU model of a symbol table.
 */
typedef struct ElfuSymtab {
  /*! All parsed symbols */
  CIRCLEQ_HEAD(Syms, ElfuSym) syms;
} ElfuSymtab;





/*!
 * ELFU model of a relocation entry.
 */
typedef struct ElfuRel {
  /*! Location to patch */
  GElf_Addr offset;


  /*! Symbol to calculate value for */
  GElf_Word sym;

  /*! Relocation type (machine specific) */
  unsigned char type;


  /*! addendUsed == 0: Use addend from relocation destination (REL).
   *  addendUsed != 0: Use explicit addend (RELA). */
  int addendUsed;

  /*! Explicit addend in RELA tables */
  GElf_Sword addend;


  /*! Element in table */
  CIRCLEQ_ENTRY(ElfuRel) elem;
} ElfuRel;



/*!
 * ELFU model of a relocation table.
 */
typedef struct ElfuReltab {
  /*! All parsed relocation entries */
  CIRCLEQ_HEAD(Rels, ElfuRel) rels;
} ElfuReltab;







/*!
 * ELFU model of an ELF section.
 *
 * Several members of the SHDR are repeated in abstract form and take
 * precedence if present. For example, if linkptr is non-NULL then
 * it points to another section and the SHDR's numeric sh_link member
 * is to be ignored and recomputed by the layouter.
 */
typedef struct ElfuScn {
  /*! SHDR as per ELF specification.
   *  Several values are ignored or recomputed from other members of
   *  ElfuScn. */
  GElf_Shdr shdr;

  /*! malloc()ed buffer containing this section's flat data backbuffer.
   *  Present for all sections that occupy space in the file. */
  char *databuf;


  /*! Pointer to a section, meaning dependent on sh_type.
   *  Preferred over sh_link if non-null. */
  struct ElfuScn *linkptr;

  /*! Pointer to a section, meaning dependent on sh_type.
   *  Preferred over sh_info if non-null. */
  struct ElfuScn *infoptr;


  /*! If this section is a clone of another section, then this
   *  points to that other section.
   *  Used during code injection to facilitate relocation. */
  struct ElfuScn *oldptr;


  /*! Translated contents of a symbol table. */
  struct ElfuSymtab symtab;

  /*! Translated contents of a relocation table. */
  struct ElfuReltab reltab;


  /*! Element linking this section into an ElfuPhdr's childScnList,
   *  or into an ElfuElf's orphanScnList. */
  CIRCLEQ_ENTRY(ElfuScn) elemChildScn;
} ElfuScn;



/*!
 * ELFU model of an ELF PHDR.
 */
typedef struct ElfuPhdr {
  /*! PHDR as per ELF specification. */
  GElf_Phdr phdr;


  /*! Sections to be shifted in file/memory together with this PHDR */
  CIRCLEQ_HEAD(ChildScnList, ElfuScn) childScnList;

  /*! PHDRs to be shifted in file/memory together with this PHDR */
  CIRCLEQ_HEAD(ChildPhdrList, ElfuPhdr) childPhdrList;


  /*! Element linking this section into an ElfuPhdr's childPhdrList */
  CIRCLEQ_ENTRY(ElfuPhdr) elemChildPhdr;

  /*! Element linking this section into an ElfuElf's phdrList */
  CIRCLEQ_ENTRY(ElfuPhdr) elem;
} ElfuPhdr;



/*!
 * ELFU model of an ELF file header
 */
typedef struct {
  /*! The ELF class -- ELFCLASS32/ELFCLASS64 */
  int elfclass;

  /*! EHDR as per ELF specification */
  GElf_Ehdr ehdr;


  /*! PHDRs that are not dependent on others.
   *  Usually, LOAD and GNU_STACK. */
  CIRCLEQ_HEAD(PhdrList, ElfuPhdr) phdrList;

  /*! Sections not in any PHDR, and thus not loaded at runtime.
   *  Usually .symtab, .strtab, .shstrtab, .debug_*, ... */
  CIRCLEQ_HEAD(OrphanScnList, ElfuScn) orphanScnList;


  /*! Pointer to ElfuScn containing .shstrtab.
   *  Parsed from EHDR for convenience. */
  ElfuScn *shstrtab;

  /*! Pointer to ElfuScn containing .symtab.
   *  Parsed from list of sections for convenience. */
  ElfuScn *symtab;
} ElfuElf;

#endif
