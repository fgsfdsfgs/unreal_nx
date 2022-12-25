#include <switch.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>

#include "utils.h"

static int nxlink_sock = -1;

void fatal_error(const char *fmt, ...) {
  PadState pad;
  va_list list;

  if (SDL_WasInit(0))
    SDL_Quit();

  // print to file
  FILE *f = fopen("fatal.log", "wb");
  if (f) {
    va_start(list, fmt);
    vfprintf(f, fmt, list);
    va_end(list);
    fclose(f);
  }

  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);

  consoleInit(NULL);

  va_start(list, fmt);
  vprintf(fmt, list);
  va_end(list);

  printf("\n\nPress A to exit.");

  consoleUpdate(NULL);

  while (appletMainLoop()) {
    padUpdate(&pad);
    const u64 keys = padGetButtonsDown(&pad);
    if (keys & HidNpadButton_A) break;
  }

  nxlink_quit();
  consoleExit(NULL);
  exit(1);
}

int file_exists(const char *fname) {
  struct stat st;
  return stat(fname, &st) == 0;
}

void nxlink_init(void) {
  nxlink_sock = nxlinkStdio();
}

void nxlink_quit(void) {
  if (nxlink_sock >= 0)
    close(nxlink_sock);
}

void *get_tpidr_el0(void) {
  void* ret;
  __asm__ ("mrs %x[data], s3_3_c13_c0_2" : [data] "=r" (ret));
  return ret;
}

void set_tpidr_el0(void *addr) {
  __asm__  ("msr s3_3_c13_c0_2, %0" : : "r"(addr));
}
