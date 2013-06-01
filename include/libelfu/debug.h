#ifndef __LIBELFU_DEBUG_H_
#define __LIBELFU_DEBUG_H_

#include <stdio.h>


#define ELFU_DEBUG(...) do { fprintf(stdout, __VA_ARGS__); } while(0)

#define ELFU_INFO(...) do { fprintf(stdout, __VA_ARGS__); } while(0)

#define ELFU_WARN(...) do { fprintf(stderr, __VA_ARGS__); } while(0)

#define ELFU_WARNELF(function_name) ELFU_WARN(function_name "() failed: %s\n", elf_errmsg(-1))

#endif
