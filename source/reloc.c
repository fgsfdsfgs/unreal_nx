#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <switch.h>
#include <solder.h>

#include "reloc.h"

void apply_user_relocs(void *elf, const user_reloc_t *relocs, const size_t num_relocs, const uintptr_t filter_lo, const uintptr_t filter_hi) {
  u8 *base = solder_get_base_addr(elf);
  for (size_t i = 0; i < num_relocs; ++i) {
    const user_reloc_t *reloc = &relocs[i];
    uintptr_t *start_addr, *end_addr;
    if (reloc->is_range) {
      start_addr = (uintptr_t *)(base + reloc->start);
      end_addr = (uintptr_t *)(base + reloc->end);
      printf("relocating range %p - %p\n", start_addr, end_addr);
    } else {
      start_addr = (uintptr_t *)(base + reloc->addr);
      end_addr = start_addr;
      printf("relocating addr  %p\n", start_addr);
    }
    for (uintptr_t *addr = start_addr; addr <= end_addr; ++addr) {
      // sanity check
      if (*addr >= filter_lo && *addr <= filter_hi)
        *addr += (uintptr_t)base;
    }
  }
}
