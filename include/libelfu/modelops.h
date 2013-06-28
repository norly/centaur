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

/*!
 * @file modelops.h
 * @brief Operations offered on libelfu's Elfu* models.
 *
 * This includes:
 *  - Allocation/initialization, teardown of objects
 *  - Iteration (*Forall)
 *  - Stats (counts, lowest/highest element, ...)
 *  - Lookups (addresses, ...)
 *  - Scripted high-level operations (reladd, detour, ...)
 */

#ifndef __LIBELFU_MODELOPS_H__
#define __LIBELFU_MODELOPS_H__

#include <elf.h>
#include <gelf.h>

#include <libelfu/types.h>


/*!
 * @brief Lookup name of a string.
 * @param symtabscn ElfuScn of symbol table.
 * @param off       Offset in string table at which name starts.
 * @result Pointer to name.
 * @result INTERNAL USE ONLY.
 *         See elfu_mSymtabSymToName() for an alternative.
 *
 */
#define ELFU_SYMSTR(symtabscn, off) ((symtabscn)->linkptr->databuf + (off))


/*!
 * @brief Lookup the address a symbol points to.
 * @param me     Entire ELF model.
 * @param msst   ElfuScn containing the symbol table this symbol is in.
 * @param sym    The symbol itself.
 * @param result Will be set to the calculated address.
 * @result 0 if *result is valid. Otherwise, *result is undefined and the
 *         address could not be resolved.
 * @note This is currently for INTERNAL USE in the relocator ONLY.
 */
int elfu_mSymtabLookupSymToAddr(ElfuElf *me, ElfuScn *msst, ElfuSym *sym, GElf_Addr *result);

/*!
 * @brief Lookup name of a symbol.
 * @param msst ElfuScn containing the symbol table this symbol is in.
 * @param sym  The symbol itself.
 * @result Pointer to name.
 */
char* elfu_mSymtabSymToName(ElfuScn *msst, ElfuSym *sym);

/*!
 * @brief Lookup a symbol by its index in a symbol table.
 * @param msst  ElfuScn containing the symbol table this symbol is in.
 * @param entry The symbol's index in the table.
 * @result Pointer to the symbol.
 */
ElfuSym* elfu_mSymtabIndexToSym(ElfuScn *msst, GElf_Word entry);

/*!
 * @brief Lookup the address a symbol points to, by the symbol name.
 * @param me   Entire ELF model.
 * @param msst ElfuScn containing the symbol table this symbol is in.
 * @param name The symbol's name.
 * @result The address the symbol points to.
 */
GElf_Addr elfu_mSymtabLookupAddrByName(ElfuElf *me, ElfuScn *msst, char *name);

/*!
 * @brief Serialize an ELF's global symbol table.
 * @param me Entire ELF model.
 */
void elfu_mSymtabFlatten(ElfuElf *me);

/*!
 * @brief Check if a global symbol table exists, and add it otherwise.
 * @param me Entire ELF model.
 */
void elfu_mSymtabAddGlobalDymtabIfNotPresent(ElfuElf *me);




/*!
 * @brief Callback for elfu_mPhdrForall().
 * @param me   Entire ELF model.
 * @param mp   Current PHDR to process.
 * @param aux1 User defined.
 * @param aux2 User defined.
 * @result NULL if iteration is to continue.
 *         Otherwise it is aborted and the handler's return value is
 *         returned by elfu_mPhdrForall() itself.
 */
typedef void* (PhdrHandlerFunc)(ElfuElf *me, ElfuPhdr *mp, void *aux1, void *aux2);

/*!
 * @brief Iterate over all PHDRs.
 * @param me   Entire ELF model.
 * @param f    Callback function.
 * @param aux1 User defined.
 * @param aux2 User defined.
 * @result NULL if all items have been processed.
 *         Otherwise the return value of the last handler function
 *         call before aborting.
 */
void* elfu_mPhdrForall(ElfuElf *me, PhdrHandlerFunc f, void *aux1, void *aux2);

/*!
 * @brief Calculate number of PHDRs in an ElfuElf.
 * @param me Entire ELF model.
 * @result Total number of PHDRs.
 */
size_t elfu_mPhdrCount(ElfuElf *me);

/*!
 * @brief Find a PHDR that covers a memory address.
 * @param me   Entire ELF model.
 * @param addr A memory address.
 * @result Pointer to a PHDR containing the given memory address.
 *         NULL if none found.
 */
ElfuPhdr* elfu_mPhdrByAddr(ElfuElf *me, GElf_Addr addr);

/*!
 * @brief Find a PHDR that covers a file offset.
 * @param me     Entire ELF model.
 * @param offset A file offset.
 * @result Pointer to a PHDR containing the given file offset.
 *         NULL if none found.
 */
ElfuPhdr* elfu_mPhdrByOffset(ElfuElf *me, GElf_Off offset);

/*!
 * @brief Find the ElfuElf's memory address and file offset
 *        extrema in terms of PHDRs.
 * @param me Entire ELF model.
 * @param lowestAddr     Will be set to PHDR containing the lowest address referenced.
 * @param highestAddr    Will be set to PHDR containing the highest address referenced.
 * @param lowestOffs     Will be set to PHDR containing the lowest offset referenced.
 * @param highestOffsEnd Will be set to PHDR containing the highest offset referenced.
 */
void elfu_mPhdrLoadLowestHighest(ElfuElf *me,
                                 ElfuPhdr **lowestAddr,
                                 ElfuPhdr **highestAddr,
                                 ElfuPhdr **lowestOffs,
                                 ElfuPhdr **highestOffsEnd);

/*!
 * @brief Update child sections' offsets in the file according to parent
 *        PHDR's offset and address.
 * @param mp Parent PHDR whose children to update.
 */
void elfu_mPhdrUpdateChildOffsets(ElfuPhdr *mp);

/*!
 * @brief Allocate and initialize a PHDR model.
 * @result Pointer to a fresh ElfuPhdr.
 *         NULL if allocation failed.
 */
ElfuPhdr* elfu_mPhdrAlloc();

/*!
 * @brief Tear down a PHDR and its children.
 * @param mp PHDR to delete.
 */
void elfu_mPhdrDestroy(ElfuPhdr* mp);




/*!
 * @brief Callback for elfu_mScnForall().
 * @param me   Entire ELF model.
 * @param ms   Current section to process.
 * @param aux1 User defined.
 * @param aux2 User defined.
 * @result NULL if iteration is to continue.
 *         Otherwise it is aborted and the handler's return value is
 *         returned by elfu_mScnForall() itself.
 */
typedef void* (SectionHandlerFunc)(ElfuElf *me, ElfuScn *ms, void *aux1, void *aux2);

/*!
 * @brief Iterate over all sections.
 * @param me   Entire ELF model.
 * @param f    Callback function.
 * @param aux1 User defined.
 * @param aux2 User defined.
 * @result NULL if all items have been processed.
 *         Otherwise the return value of the last handler function
 *         call before aborting.
 */
void* elfu_mScnForall(ElfuElf *me, SectionHandlerFunc f, void *aux1, void *aux2);

/*!
 * @brief Calculate number of sections in an ElfuElf.
 * @param me Entire ELF model.
 * @result Total number of sections.
 */
size_t elfu_mScnCount(ElfuElf *me);

/*!
 * @brief Calculate index a section would currently have in its ElfuElf.
 * @param me Entire ELF model.
 * @param ms A section in *me.
 * @result Estimated index.
 */
size_t elfu_mScnIndex(ElfuElf *me, ElfuScn *ms);

/*!
 * @brief Find a cloned section by its oldscn value.
 * @param me     Entire ELF model.
 * @param oldscn Original section to find the clone of.
 * @result A section that is a clone of *oldscn.
 *         NULL if none found.
 */
ElfuScn* elfu_mScnByOldscn(ElfuElf *me, ElfuScn *oldscn);

/*!
 * @brief Get a section's name.
 * @param me Entire ELF model.
 * @param ms A section in *me.
 * @result Pointer to the section's name in .shstrtab.
 */
char* elfu_mScnName(ElfuElf *me, ElfuScn *ms);

/*!
 * @brief Allocate an array of pointers to all sections,
 *        and sort them by offset.
 * @param me    Entire ELF model.
 * @param count Where to write total count of sections to.
 * @result Pointer to the section's name in .shstrtab.
 *         NULL if an error occurred.
 */
ElfuScn** elfu_mScnSortedByOffset(ElfuElf *me, size_t *count);

/*!
 * @brief Enlarge a section's buffer and append data to it.
 * @param ms Section to append to.
 * @param buf Source buffer with data to append.
 * @param len Length of source buffer.
 * @result 0 if successful.
 *         Anything else indicates an error.
 */
int elfu_mScnAppendData(ElfuScn *ms, void *buf, size_t len);

/*!
 * @brief Allocate and initialize a section model.
 * @result Pointer to a fresh ElfuScn.
 *         NULL if allocation failed.
 */
ElfuScn* elfu_mScnAlloc();

/*!
 * @brief Tear down a section and associated buffers.
 * @param ms Section to delete.
 */
void elfu_mScnDestroy(ElfuScn* ms);




/*!
 * @brief Allocate and initialize an ELF file model.
 * @result Pointer to a fresh ElfuElf.
 *         NULL if allocation failed.
 */
ElfuElf* elfu_mElfAlloc();

/*!
 * @brief Tear down an ELF file model and associated structures.
 * @param me ElfuElf to destroy.
 */
void elfu_mElfDestroy(ElfuElf* me);




/*!
 * @brief Find a LOAD segment to inject into, and expand/create it.
 * @param me      Entire ELF model.
 * @param size    Size of data to inject.
 * @param align   Alignment requirement of new data.
 * @param w       Whether the new data should be writable when loaded.
 * @param x       Whether the new data should be executable when loaded.
 * @param injPhdr Will be set to the identified target PHDR.
 * @result The address where the data will be located.
 */
GElf_Addr elfu_mLayoutGetSpaceInPhdr(ElfuElf *me, GElf_Word size,
                                     GElf_Word align, int w, int x,
                                     ElfuPhdr **injPhdr);
/*!
 * @brief Re-layout an ELF file so nothing overlaps that should not.
 *        Also, recalculate various offsets and sizes where necessary.
 * @param me Entire ELF model.
 */
int elfu_mLayoutAuto(ElfuElf *me);




/*!
 * @brief Lookup the address of a function in the PLT by its name.
 * @param me   Entire ELF model.
 * @param name The function's name.
 * @param result Will be set to the calculated address.
 * @result 0 if *result is valid. Otherwise, *result is undefined and the
 *         address could not be resolved.
 */
int elfu_mDynLookupPltAddrByName(ElfuElf *me, char *name, GElf_Addr *result);

/*!
 * @brief Lookup the address of a dynamically loaded variable by its name.
 * @param me   Entire ELF model.
 * @param name The variable's name.
 * @param result Will be set to the calculated address.
 * @result 0 if *result is valid. Otherwise, *result is undefined and the
 *         address could not be resolved.
 */
int elfu_mDynLookupReldynAddrByName(ElfuElf *me, char *name, GElf_Addr *result);




/*!
 * @brief Relocate a section.
 * @param metarget ELF model containing cloned section.
 * @param mstarget Cloned section to be relocated.
 * @param msrt     Section in original ELF model,
 *                 containing relocation table.
 * @result 0 if successful.
 *         Anything else indicates an error.
 * @note This is currently for INTERNAL USE in Reladd ONLY.
 */
int elfu_mRelocate(ElfuElf *metarget, ElfuScn *mstarget, ElfuScn *msrt);




/*!
 * @brief Perform a few sanity checks.
 * @param me Entire ELF model.
 * @result 0 if successful.
 *         Anything else indicates an error.
 */
int elfu_mCheck(ElfuElf *me);




/*!
 * @brief Dump contents of a PHDR to stdout.
 * @param me Entire ELF model.
 * @param mp PHDR to dump.
 */
void elfu_mDumpPhdr(ElfuElf *me, ElfuPhdr *mp);

/*!
 * @brief Dump details of a section to stdout.
 * @param me Entire ELF model.
 * @param ms Section to dump.
 */
void elfu_mDumpScn(ElfuElf *me, ElfuScn *ms);

/*!
 * @brief Dump contents of an entire ELF file model to stdout.
 * @param me Entire ELF model to dump.
 */
void elfu_mDumpElf(ElfuElf *me);




/*!
 * @brief Parse an ELF file to a libelfu model via libelf.
 * @param e libelf handle to source file.
 * @result NULL if an error occurred, a fresh ElfuElf otherwise.
 */
ElfuElf* elfu_mFromElf(Elf *e);

/*!
 * @brief Serialize a libelfu model to an ELF file via libelf.
 * @param me Entire ELF model.
 * @param e  libelf handle to destination file.
 */
void elfu_mToElf(ElfuElf *me, Elf *e);




/*!
 * @brief Inject contents of an object file into an executable.
 * @param me   Destination ELF model.
 * @param mrel Source ELF model.
 * @result 0 if successful.
 *         Anything else indicates an error.
 */
int elfu_mReladd(ElfuElf *me, const ElfuElf *mrel);




/*!
 * @brief Overwrite a location with an unconditional jump.
 * @param me   Entire ELF model.
 * @param from Memory address to overwrite at.
 * @param to   Memory address to jump to.
 */
void elfu_mDetour(ElfuElf *me, GElf_Addr from, GElf_Addr to);


#endif
