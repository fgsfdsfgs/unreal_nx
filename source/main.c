#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include <switch.h>
#include <solder.h>

#include "utils.h"
#include "reloc.h"
#include "exports.h"
#include "hooks.h"

static __thread u8 fake_tls[0x1000];

static void *main_elf;
static void *engine_so;
static void *core_so;

void userAppInit(void) {
  socketInitializeDefault();
}

void userAppExit(void) {
  // free the modules after all the atexit() handlers are done
  solder_quit();
  nxlink_quit();
  socketExit();
}

// patch-relocations for UnrealLinux.bin
static const user_reloc_t main_elf_relocs[] = {
  USER_RELOC_RANGE(0x43F460, 0x43F4A8), // z_errmsg and other strings
  USER_RELOC_RANGE(0x43F4B8, 0x43FB40), // various vtables
  USER_RELOC_ADDR (0x440568),           // GPackageInternal ptr in GPackageSDL2Launch
  USER_RELOC_ADDR (0x440570),           // vtable ptr in Malloc
};

static int patch_instr(void *mod, const u64 ofs, const u32 src, const u32 dst) {
  return solder_patch_offset(mod, ofs, &src, &dst, sizeof(dst));
}

static inline void apply_patches(void) {
  int err = 0;
  printf("applying module patches\n");

  // Core.so
  err += solder_hook_function(core_so, "_Z9appThrowfPKwz", hook_app_throw);
  if (err != 0)
    fatal_error("Could not patch Core.so. Check if you have the right version.");

  // Engine.so
  err += patch_instr(engine_so, 0x1590B4, 0x97FD2FD3, 0xAA1F03E0); // replace StaticLoadClass for physics with a `mov x0, #0`
  err += patch_instr(engine_so, 0x1590FC, 0xF9404A60, 0xAA1F03E0); // replace getting default physics class with a `mov x0, #0`
  if (err != 0)
    fatal_error("Could not patch Engine.so. Check if you have the right version.");
}

int main(int argc, char *argv[]) {
  // if we've been passed any arguments, that was probably from nxlink
  if (argc > 1)
    nxlink_init();

  // init solder asap
  if (solder_init(SOLDER_ALLOW_UNDEFINED) < 0)
    fatal_error("Could not init solder:\n%s", solder_dlerror());

  // if this is ever set to 1 it'll just explode in a glTexSubImage2D call
  unsetenv("MESA_NO_ERROR");

  // it'll try to access() this even if NOHOMEDIR is enabled
  setenv("HOME", "../Home", 1);

  // HACK: unreal wants these as symbols
  export_stdin = stdin;
  export_stdout = stdout;
  export_stderr = stderr;

  // HACK: switch appears to use tpidrro_el0 for TP storage, but unreal uses tpidr_el0, so we feed it a tls buffer
  // libsolder only "properly" handles local-exec TLS, so this will hopefully just resolve itself
  set_tpidr_el0(fake_tls);

  if (chdir("./SystemARM") < 0)
    fatal_error("Could not cd into SystemARM:\n%d", errno);

  // mesa_enable_debug();

  // load Core.so and Engine.so manually so we can patch them before mapping
  core_so = solder_dlopen("Core.so", SOLDER_NO_AUTOLOAD | SOLDER_LAZY);
  if (!core_so)
    fatal_error("Could not load Core.so:\n%s", solder_dlerror());
  engine_so = solder_dlopen("Engine.so", SOLDER_NO_AUTOLOAD | SOLDER_LAZY);
  if (!engine_so)
    fatal_error("Could not load Engine.so:\n%s", solder_dlerror());

  // patch them
  apply_patches();

  // force solder to relocate them
  solder_dlsym(core_so, "?");
  solder_dlsym(engine_so, "?");

  // load and link the main executable
  main_elf = solder_dlopen("UnrealLinux.bin", SOLDER_NO_AUTOLOAD | SOLDER_NOW);
  if (!main_elf)
    fatal_error("Could not load UnrealLinux.bin:\n%s", solder_dlerror());

  // since this is a ET_EXEC, we'll have to patch offsets in a bunch of shit
  apply_user_relocs(main_elf, main_elf_relocs, sizeof(main_elf_relocs) / sizeof(*main_elf_relocs), 0x400000, 0x450000);

  solder_dlerror();
  void (*start_func)(void) = solder_get_entry_addr(main_elf);
  if (!start_func)
    fatal_error("Could not find entry point:\n%s", solder_dlerror());

  // get the absolute path to UnrealLinux.bin to use as argv[0]
  char tmp_path[1024] = { 0 };
  getcwd(tmp_path, sizeof(tmp_path));
  strncat(tmp_path, "/UnrealLinux.bin", sizeof(tmp_path) - 1);

  fake_argv[fake_argc++] = tmp_path + 5; // skip "sdmc:"
  for (; fake_argc < argc && fake_argc < 16; ++fake_argc)
    fake_argv[fake_argc] = argv[fake_argc];
  fake_argv[fake_argc++] = "-NOHOMEDIR"; // store configs locally
  fake_argv[fake_argc++] = "-LOG"; // enable logging
  fake_argv[fake_argc] = NULL;

  printf("time for start() %p argc=%d fake_argc=%d\n", start_func, argc, fake_argc);

  start_func();

  printf("main() returned!\n");

  return 0;
}
