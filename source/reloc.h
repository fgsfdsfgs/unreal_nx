#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

typedef struct user_reloc_s {
  int is_range;
  union {
    struct { uintptr_t addr; };
    struct { uintptr_t start, end; };
  };
} user_reloc_t;

#define USER_RELOC_ADDR(a) ((user_reloc_t){ .is_range = 0, .addr = (a) })
#define USER_RELOC_RANGE(a, b) ((user_reloc_t){ .is_range = 1, .start = (a), .end = (b) })

void apply_user_relocs(void *elf, const user_reloc_t *relocs, const size_t num_relocs, const uintptr_t filter_lo, const uintptr_t filter_hi);
