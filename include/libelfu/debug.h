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

#ifndef __LIBELFU_DEBUG_H_
#define __LIBELFU_DEBUG_H_

#include <stdio.h>


#define ELFU_DEBUG(...) do { fprintf(stdout, __VA_ARGS__); } while(0)

#define ELFU_INFO(...) do { fprintf(stdout, __VA_ARGS__); } while(0)

#define ELFU_WARN(...) do { fprintf(stderr, __VA_ARGS__); } while(0)

#define ELFU_WARNELF(function_name) ELFU_WARN(function_name "() failed: %s\n", elf_errmsg(-1))

#endif
