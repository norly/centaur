/*!
 * @file elfops.h
 * @brief Operations offered by libelfu on libelf handles
 *
 * This includes:
 *  - Checks
 *  - Post-processing for ELF specification compliance
 */

#ifndef __LIBELFU_ELFOPS_H_
#define __LIBELFU_ELFOPS_H_

#include <libelf.h>
#include <gelf.h>

#include <libelfu/types.h>


/*!
 * @brief Perform a large array of sanity checks.
 * @param e libelf handle to file to be checked.
 * @result 0 if successful.
 *         Anything else indicates an error.
 * @note If a file does not pass these checks,
 *       then it cannot be processed by libelfu.
 */
int elfu_eCheck(Elf *e);



/*!
 * @brief Reorder PHDRs to comply with ELF specification.
 * @param e libelf handle to file to be post-processed.
 */
void elfu_eReorderPhdrs(Elf *e);


#endif
