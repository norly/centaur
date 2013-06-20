#include <assert.h>
#include <libelfu/libelfu.h>


static char* segmentTypeStr(Elf32_Word p_type)
{
  switch(p_type) {
    case PT_NULL: /* 0 */
      return "NULL";
    case PT_LOAD: /* 1 */
      return "LOAD";
    case PT_DYNAMIC: /* 2 */
      return "DYNAMIC";
    case PT_INTERP: /* 3 */
      return "INTERP";
    case PT_NOTE: /* 4 */
      return "NOTE";
    case PT_SHLIB: /* 5 */
      return "SHLIB";
    case PT_PHDR: /* 6 */
      return "PHDR";
    case PT_TLS: /* 7 */
      return "TLS";
    case PT_NUM: /* 8 */
      return "NUM";
    case PT_GNU_EH_FRAME: /* 0x6474e550 */
      return "GNU_EH_FRAME";
    case PT_GNU_STACK: /* 0x6474e551 */
      return "GNU_STACK";
    case PT_GNU_RELRO: /* 0x6474e552 */
      return "GNU_RELRO";
  }

  return "-?-";
}

static char* sectionTypeStr(Elf32_Word sh_type)
{
  switch(sh_type) {
    case SHT_NULL: /* 0 */
      return "NULL";
    case SHT_PROGBITS: /* 1 */
      return "PROGBITS";
    case SHT_SYMTAB: /* 2 */
      return "SYMTAB";
    case SHT_STRTAB: /* 3 */
      return "STRTAB";
    case SHT_RELA: /* 4 */
      return "RELA";
    case SHT_HASH: /* 5 */
      return "HASH";
    case SHT_DYNAMIC: /* 6 */
      return "DYNAMIC";
    case SHT_NOTE: /* 7 */
      return "NOTE";
    case SHT_NOBITS: /* 8 */
      return "NOBITS";
    case SHT_REL: /* 9 */
      return "REL";
    case SHT_SHLIB: /* 10 */
      return "SHLIB";
    case SHT_DYNSYM: /* 11 */
      return "DYNSYM";
    case SHT_INIT_ARRAY: /* 14 */
      return "INIT_ARRAY";
    case SHT_FINI_ARRAY: /* 15 */
      return "FINI_ARRAY";
    case SHT_PREINIT_ARRAY: /* 16 */
      return "PREINIT_ARRAY";
    case SHT_GROUP: /* 17 */
      return "SHT_GROUP";
    case SHT_SYMTAB_SHNDX: /* 18 */
      return "SYMTAB_SHNDX";
    case SHT_NUM: /* 19 */
      return "NUM";

    case SHT_GNU_ATTRIBUTES: /* 0x6ffffff5 */
      return "GNU_ATTRIBUTES";
    case SHT_GNU_HASH: /* 0x6ffffff6 */
      return "GNU_HASH";
    case SHT_GNU_LIBLIST: /* 0x6ffffff7 */
      return "GNU_LIBLIST";
    case SHT_GNU_verdef: /* 0x6ffffffd */
      return "GNU_verdef";
    case SHT_GNU_verneed: /* 0x6ffffffe */
      return "GNU_verneed";
    case SHT_GNU_versym: /* 0x6fffffff */
      return "GNU_versym";
  }

  return "-?-";
}




void elfu_mDumpPhdr(ElfuElf *me, ElfuPhdr *mp)
{
  assert(me);
  assert(mp);

  ELFU_INFO("%12s %8x %8x %8x %8x %8x %8x %8x %8x\n",
            segmentTypeStr(mp->phdr.p_type),
            (unsigned)mp->phdr.p_type,
            (unsigned)mp->phdr.p_offset,
            (unsigned)mp->phdr.p_vaddr,
            (unsigned)mp->phdr.p_paddr,
            (unsigned)mp->phdr.p_filesz,
            (unsigned)mp->phdr.p_memsz,
            (unsigned)mp->phdr.p_flags,
            (unsigned)mp->phdr.p_align);

  if (!CIRCLEQ_EMPTY(&mp->childPhdrList)) {
    ElfuPhdr *mpc;

    ELFU_INFO("      -> Child PHDRs:\n");
    CIRCLEQ_FOREACH(mpc, &mp->childPhdrList, elemChildPhdr) {
      ELFU_INFO("        * %-8s @ %8x\n",
                segmentTypeStr(mpc->phdr.p_type),
                (unsigned)mpc->phdr.p_vaddr);
    }
  }

  if (!CIRCLEQ_EMPTY(&mp->childScnList)) {
    ElfuScn *msc;

    ELFU_INFO("      -> Child sections:\n");
    CIRCLEQ_FOREACH(msc, &mp->childScnList, elemChildScn) {
      ELFU_INFO("        * %-17s @ %8x\n",
                elfu_mScnName(me, msc),
                (unsigned)msc->shdr.sh_addr);
    }
  }
}


void elfu_mDumpScn(ElfuElf *me, ElfuScn *ms)
{
  char *namestr, *typestr, *linkstr, *infostr;

  assert(me);
  assert(ms);

  namestr = elfu_mScnName(me, ms);
  typestr = sectionTypeStr(ms->shdr.sh_type);
  linkstr = ms->linkptr ? elfu_mScnName(me, ms->linkptr) : "";
  infostr = ms->infoptr ? elfu_mScnName(me, ms->infoptr) : "";

  ELFU_INFO("%-17s %-15s %8x %9x %8x %2x %2x %2d %-17s %-17s\n",
            namestr,
            typestr,
            (unsigned)ms->shdr.sh_addr,
            (unsigned)ms->shdr.sh_offset,
            (unsigned)ms->shdr.sh_size,
            (unsigned)ms->shdr.sh_entsize,
            (unsigned)ms->shdr.sh_flags,
            (unsigned)ms->shdr.sh_addralign,
            linkstr,
            infostr);
}


void elfu_mDumpEhdr(ElfuElf *me)
{
  assert(me);

  ELFU_INFO("ELF header:\n");
  ELFU_INFO("   %d-bit ELF object\n", me->elfclass == ELFCLASS32 ? 32 : 64);

  ELFU_INFO("   EHDR:\n");

  ELFU_INFO("     e_type       %8x\n", me->ehdr.e_type);
  ELFU_INFO("     e_machine    %8x\n", me->ehdr.e_machine);
  ELFU_INFO("     e_version    %8x\n", me->ehdr.e_version);
  ELFU_INFO("     e_entry      %8x\n", (unsigned)me->ehdr.e_entry);
  ELFU_INFO("     e_phoff      %8x\n", (unsigned)me->ehdr.e_phoff);
  ELFU_INFO("     e_shoff      %8x\n", (unsigned)me->ehdr.e_shoff);
  ELFU_INFO("     e_flags      %8x\n", me->ehdr.e_flags);
  ELFU_INFO("     e_ehsize     %8x\n", me->ehdr.e_ehsize);
  ELFU_INFO("     e_phentsize  %8x\n", me->ehdr.e_phentsize);
  ELFU_INFO("     e_shentsize  %8x\n", me->ehdr.e_shentsize);

  ELFU_INFO("   shstrtab: %s\n", me->shstrtab ? elfu_mScnName(me, me->shstrtab) : "(none)");
}



static void* subScnDump(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2)
{
  (void) aux1;
  (void) aux2;

  printf(" [%4d] ", elfu_mScnIndex(me, ms));
  elfu_mDumpScn(me, ms);

  return NULL;
}


void elfu_mDumpElf(ElfuElf *me)
{
  ElfuPhdr *mp;
  ElfuScn *ms;
  size_t i;

  assert(me);


  elfu_mDumpEhdr(me);
  ELFU_INFO("\n");


  ELFU_INFO("Segments:\n");
  ELFU_INFO("     #        (type)   p_type p_offset  p_vaddr  p_paddr p_filesz  p_memsz  p_flags  p_align\n");
  ELFU_INFO("       |            |        |        |        |        |        |        |        |        \n");
  i = 0;
  CIRCLEQ_FOREACH(mp, &me->phdrList, elem) {
    printf(" [%4d] ", i);
    elfu_mDumpPhdr(me, mp);
    i++;
  }
  ELFU_INFO("\n");


  ELFU_INFO("Orphaned sections:\n");
  CIRCLEQ_FOREACH(ms, &me->orphanScnList, elemChildScn) {
    ELFU_INFO("        * %-17s @ %8x\n",
              elfu_mScnName(me, ms),
              (unsigned)ms->shdr.sh_addr);
  }
  ELFU_INFO("\n");


  ELFU_INFO("Sections:\n");
  ELFU_INFO("     #  Name              sh_type          sh_addr sh_offset  sh_size ES Fl Al sh_link           sh_info          \n");
  ELFU_INFO("       |                 |               |        |         |        |  |  |  |                 |                 \n");
  elfu_mScnForall(me, subScnDump, &i, NULL);
  ELFU_INFO("\n");
}
