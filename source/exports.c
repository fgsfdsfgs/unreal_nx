#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <setjmp.h>
#include <wchar.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <iconv.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h>
#include <malloc.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>

#include <switch.h>
#include <solder.h>

#include <SDL2/SDL.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <AL/alure.h>

#include <xmp.h>

#include "utils.h"
#include "exports.h"

#define TIMER_FD 0x56435643
#define TRIGGER_THRESHOLD 0x6000
#define AXIS_DEADZONE 0x400

static u64 start_tick;

static SDL_GameController *sdl_ctrl;
static u8 sdl_keys[SDL_NUM_SCANCODES];

FILE *export_stdin;
FILE *export_stdout;
FILE *export_stderr;

char *fake_argv[MAX_FAKE_ARGC];
int fake_argc;

// local libstdc++ crap that needs to be exported
extern int __cxa_atexit(void *, void *, void *) __attribute__((used));
extern void _Unwind_Resume(void *) __attribute__((used));

// these are not visible by default for some reason
extern int usleep(useconds_t u) __attribute__((used));
extern unsigned sleep(unsigned int s) __attribute__((used));

// can't be arsed with GNU macros
extern void sincos(double, double *, double *);
extern void sincosf(float, float *, float *);

/* general libc stuff */

// general purpose dummy function
static void dummy_fn(void) { }

void __assert_fail(const char *expr, const char *file, unsigned int line, const char *func) {
  fatal_error("ASSERTION FAILED AT %s:%u (%s):\n`%s`", file, line, func, expr);
}

// not actually used
int pthread_setaffinity_np(pthread_t thr, size_t cssize, const void *cs) { return ENOSYS; }

// unsupported
int pthread_kill(pthread_t thread, int sig) { return -1; }

// this is pretty much a no-op for our purposes
char *realpath(const char *restrict path, char *restrict resolved_path) {
  printf("realpath(%s) called\n", path);
  if (!path || !resolved_path) return NULL;
  strcpy(resolved_path, path);
  return resolved_path;
}

const unsigned short **__ctype_b_loc(void) {
  static const unsigned short tab[384] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
    0x200, 0x320, 0x220, 0x220, 0x220, 0x220, 0x200, 0x200,
    0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
    0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
    0x160, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0,
    0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0,
    0x8d8, 0x8d8, 0x8d8, 0x8d8, 0x8d8, 0x8d8, 0x8d8, 0x8d8,
    0x8d8, 0x8d8, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0,
    0x4c0, 0x8d5, 0x8d5, 0x8d5, 0x8d5, 0x8d5, 0x8d5, 0x8c5,
    0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5,
    0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5, 0x8c5,
    0x8c5, 0x8c5, 0x8c5, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x4c0,
    0x4c0, 0x8d6, 0x8d6, 0x8d6, 0x8d6, 0x8d6, 0x8d6, 0x8c6,
    0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6,
    0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6, 0x8c6,
    0x8c6, 0x8c6, 0x8c6, 0x4c0, 0x4c0, 0x4c0, 0x4c0, 0x200,
  };
  static const unsigned short *tabptr = tab + 128;
  return &tabptr;
}

// TODO: maybe return username
char *getlogin(void) { 
  static char username[] = "Player";
  return username;
}

// probably don't need any real values
long pathconf(const char *path, int name) {
  printf("pathconf(%s, %d) called\n", path, name);
  return -1;
}

// only 2 config vars are actually read
long sysconf(int name) {
  printf("sysconf(%d) called\n", name);
  switch (name) {
    case 30: // PAGESIZE
      return 0x1000;
    case 83: // NPROCESSORS
      return 3; // as far as I can tell this is only used for setaffinity
    default:
      errno = EINVAL;
      return -1;
  }
}

// this is used just to get the total memory size
int sysinfo(unsigned char *info) {
  extern char *fake_heap_start;
  extern char *fake_heap_end;
  const u64 memunit = 1024; // KB
  const u64 memsize = (fake_heap_end - fake_heap_start) / memunit;
  *(u64 *)(info + 0x20) = memsize; // info->totalram
  *(u64 *)(info + 0x68) = memunit; // info->mem_unit
  return 0;
}

// this is used for tracking max rss only
int getrusage(int who, struct rusage *ru) {
  static size_t max_rss = 0;
  if (who == RUSAGE_SELF) {
    // feed it max malloc'd space instead
    struct mallinfo mi = mallinfo();
    if (mi.uordblks > max_rss)
      max_rss = mi.uordblks;
    // the field we need should be right after the fields we have
    *(long long *)&ru[1] = max_rss;
    return 0;
  }
  return -1;
}

// all the multi-process crap is not supported
pid_t fork(void) { errno = ENOSYS; return -1; }
int execve(const char *filename, char *const argv [], char *const envp[]) { errno = ENOSYS; return -1; }
int execl(const char *path, const char *arg, ...) { errno = ENOSYS; return -1; }
pid_t waitpid(pid_t pid, int *status, int options) { errno = ENOSYS; return -1; }
int system(const char *str) { errno = ENOSYS; return -1; }

// neither is most of syscall(), but
long syscall(long id, ...) {
  printf("syscall(%ld) called\n", id);
  if (id == 241 /* SYS_perf_event_open */) {
    // it's going to use this event for the perf timer
    start_tick = armGetSystemTick();
    printf(" - timer started at %lu\n", start_tick);
    return TIMER_FD;
  }
  errno = ENOSYS;
  return -1;
}

// signals can go fuck themselves tbh but we actually do have limited support for them
int sigaction(int sig, const struct sigaction *ap, struct sigaction *op) {
  // we wanna catch SIGILL and SIGSEGV ourselves
  if (ap && ap->sa_handler && sig != SIGILL && sig != SIGSEGV) {
    void *old_handler = signal(sig, ap->sa_handler);
    if (old_handler == SIG_ERR)
      return -1;
    return 0;
  }
  errno = EINVAL;
  return -1;
}

// used in appGetLocalIP, which falls back to 0.0.0.0
int getifaddrs(void **pp) { errno = ENOSYS; return -1; }
void freeifaddrs(void *p) { }

// long doubles are handled via soft floats for some reason
int __eqtf2(long double a, long double b) { return a != b; }
int __unordtf2(long double a, long double b) { return a - b; }
long double __addtf3(long double a, long double b) { return a + b; }
long double __subtf3(long double a, long double b) { return a - b; }
long double __multf3(long double a, long double b) { return a * b; }
long double __floatsitf(int i) { return (long double)i; }
long double __floatunsitf(unsigned int i) { return (long double)i; }
unsigned int __fixunstfsi(long double x) { return (unsigned int)x; }
int __fixtfsi(long double x) { return (int)x; }
long double __extenddftf2(double x) { return (long double)x; }
long double __netf2(long double a, long double b) { return a != b; }

// can't backtrace from here
int backtrace(void **buffer, int size) { return 0; }
char **backtrace_symbols(void *const *buffer, int size) { return NULL; }

// don't have these
void *getpwent(void) { return NULL; }
uid_t getuid(void) { return 1; }

// h_errno is threadlocal here so I wonder if this will work correctly
int *__h_errno_location(void) { return &h_errno; }

// these are macros in our libc
static int tolower_fn(int c) { return tolower(c); }
static int toupper_fn(int c) { return toupper(c); }
static int sigemptyset_fn(sigset_t *set) { return sigemptyset(set); }

// their timespec is different
struct timespec64 {
  s64 tv_sec;
  s64 tv_nsec;
};
static int wrap_clock_gettime(clockid_t id, struct timespec64 *tp) {
  struct timespec tv;
  const int ret = clock_gettime(CLOCK_REALTIME, &tv);
  tp->tv_sec = tv.tv_sec;
  tp->tv_nsec = tv.tv_nsec;
  return ret;
}

// their struct stat is different
struct stat64 {
  s64 st_dev;
  s64 st_ino;
  s64 st_nlink;
  s32 st_mode;
  s32 st_uid;
  s32 st_gid;
  s32 __pad0;
  s64 st_rdev;
  u64 st_size;
  u64 st_blksize;
  u64 st_blocks;
  struct timespec64 st_atim;
  struct timespec64 st_mtim;
  struct timespec64 st_ctim;
  u64 __pad1[3];
};
// some weird fucking posix versioning thing
int __xstat(int ver, char *restrict path, struct stat64 *sbuf) { 
  struct stat s;
  const int ret = stat(path, &s);
  if (ret == 0) {
    sbuf->st_dev = s.st_dev;
    sbuf->st_ino = s.st_ino;
    sbuf->st_nlink = s.st_nlink;
    sbuf->st_mode = s.st_mode;
    sbuf->st_uid = s.st_uid;
    sbuf->st_gid = s.st_gid;
    sbuf->st_rdev = s.st_rdev;
    sbuf->st_size = s.st_size;
    sbuf->st_blksize = s.st_blksize;
    sbuf->st_blocks = s.st_blocks;
    sbuf->st_atim.tv_sec = s.st_atim.tv_sec;
    sbuf->st_atim.tv_nsec = s.st_atim.tv_nsec;
    sbuf->st_mtim.tv_sec = s.st_atim.tv_sec;
    sbuf->st_mtim.tv_nsec = s.st_atim.tv_nsec;
    sbuf->st_ctim.tv_sec = s.st_atim.tv_sec;
    sbuf->st_ctim.tv_nsec = s.st_atim.tv_nsec;
  }
  return ret;
}

// their struct dirent is different
struct dirent64 {
  s64 d_ino;
  u64 d_off;
  u16 d_reclen;
  u8 d_type;
  char d_name[256 + 1];
};
static struct dirent64 *wrap_readdir(DIR *dirp) {
  // statically allocating this is allowed by spec I think
  static struct dirent64 udent = { .d_reclen = sizeof(udent) };
  struct dirent *dent = readdir(dirp);
  if (dent) {
    udent.d_ino = dent->d_ino;
    udent.d_type = dent->d_type;
    strncpy(udent.d_name, dent->d_name, 256);
    return &udent;
  } else {
    return NULL;
  }
}

static size_t wrap_read(int fd, void *buf, size_t count) {
  if (fd == TIMER_FD) {
    // it's trying to read from the perf event fd, give it the time
    *(u64 *)buf = armGetSystemTick() - start_tick;
    return count;
  }
  // normal read
  return read(fd, buf, count);
}

/* socket stuff: have to wrap these because of differing structs and constants */

struct sockaddr_linux {
  u16 sa_family;
  u8 sa_data[14];
};

struct sockaddr_in_linux {
  u16 sin_family;
  u16 sin_port;
  struct in_addr sin_addr;
  u8 sin_zero[8];
};

static inline int sockopt_cvt(const int opt) {
  switch (opt) {
    case 2:  return SO_REUSEADDR;
    case 3:  return SO_TYPE;
    case 4:  return SO_ERROR;
    case 5:  return SO_DONTROUTE;
    case 6:  return SO_BROADCAST;
    case 7:  return SO_SNDBUF;
    case 8:  return SO_RCVBUF;
    case 9:  return SO_KEEPALIVE;
    case 10: return SO_OOBINLINE;
    default:
      printf("sockopt_cvt(): unknown option %d\n", opt);
      return -1;
  }
}

static inline void laddr_to_saddr(struct sockaddr *dst, const struct sockaddr_linux *src) {
  if (dst && src) {
    // it only uses sockaddr_in anyway
    struct sockaddr_in *saddr_in = (struct sockaddr_in *)dst; 
    const struct sockaddr_in_linux *laddr_in = (const struct sockaddr_in_linux *)src;
    saddr_in->sin_len = sizeof(*saddr_in);
    saddr_in->sin_family = laddr_in->sin_family;
    saddr_in->sin_port = laddr_in->sin_port;
    saddr_in->sin_addr = laddr_in->sin_addr;
  }
}

static inline void saddr_to_laddr(struct sockaddr_linux *dst, const struct sockaddr *src) {
  if (dst && src) {
    // it only uses sockaddr_in anyway
    struct sockaddr_in_linux *laddr_in = (struct sockaddr_in_linux *)dst; 
    const struct sockaddr_in *saddr_in = (const struct sockaddr_in *)src;
    laddr_in->sin_family = saddr_in->sin_family;
    laddr_in->sin_port = saddr_in->sin_port;
    laddr_in->sin_addr = saddr_in->sin_addr;
  }
}

static int wrap_connect(int fd, const struct sockaddr_linux *addr, socklen_t len) {
  struct sockaddr saddr = { 0 };
  laddr_to_saddr(&saddr, addr);
  return connect(fd, &saddr, len);
}

static int wrap_bind(int fd, const struct sockaddr_linux *addr, socklen_t len) {
  struct sockaddr saddr = { 0 };
  laddr_to_saddr(&saddr, addr);
  return bind(fd, &saddr, len);
}

static ssize_t wrap_sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr_linux *addr, socklen_t addr_len) {
  struct sockaddr saddr = { 0 };
  laddr_to_saddr(&saddr, addr);
  return sendto(fd, buf, n, flags, &saddr, addr_len);
}

static ssize_t wrap_recvfrom(int fd, void *buf, size_t n, int flags, struct sockaddr_linux *addr, socklen_t *addr_len) {
  struct sockaddr saddr = { 0 };
  const int rc = recvfrom(fd, buf, n, flags, &saddr, addr_len);
  saddr_to_laddr(addr, &saddr);
  return rc;
}

static int wrap_accept(int fd, struct sockaddr_linux *addr, socklen_t *addr_len) {
  struct sockaddr saddr = { 0 };
  laddr_to_saddr(&saddr, addr);
  return accept(fd, &saddr, addr_len);
}

static int wrap_getsockname(int fd, struct sockaddr_linux *addr, socklen_t *len) {
  struct sockaddr saddr = { 0 };
  const int rc = getsockname(fd, &saddr, len);
  saddr_to_laddr(addr, &saddr);
  return rc;
}

static int wrap_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
  if (level == 1) level = SOL_SOCKET; // this is also different for some reason
  optname = sockopt_cvt(optname);
  if (optname < 0) return -1;
  return setsockopt(sockfd, level, optname, optval, optlen);
}

static int wrap_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
  if (level == 1) level = SOL_SOCKET; // this is also different for some reason
  optname = sockopt_cvt(optname);
  if (optname < 0) return -1;
  return getsockopt(sockfd, level, optname, optval, optlen);
}

static int wrap_fcntl(int fd, int cmd, int flag, ...) {
  if (cmd == F_SETFL && (flag & 0x800)) {
    flag &= ~0x800;
    flag |= O_NONBLOCK;
    return fcntl(fd, cmd, flag);
  }
  return fcntl(fd, cmd, flag);
}

/* SDL/GL stuff */

static int wrap_SDL_GL_SetAttribute(SDL_GLattr attr, int val) {
  int ret = -1;
  switch (attr) {
    case SDL_GL_MULTISAMPLEBUFFERS:
    case SDL_GL_MULTISAMPLESAMPLES:
    case SDL_GL_DOUBLEBUFFER:
      printf("SDL_GL_SetAttribute(%x, %d) - inhibited\n", attr, val);
      break;
    default:
      ret = SDL_GL_SetAttribute(attr, val);
      break;
  }
  return ret;
}

static SDL_Window *wrap_SDL_CreateWindow(const char *title, int x, int y, int w, int h, u32 flags) {
  // drop any extra flags it might be passing in
  SDL_Window *win = SDL_CreateWindow(title, 0, 0, w, h, flags & SDL_WINDOW_OPENGL);
  printf("SDL_CreateWindow(`%s`, %d, %d, %d, %d, 0x%08x) -> %p\n", title, x, y, w, h, flags, win);
  if (!win) printf(" - failed: %s\n", SDL_GetError());
  return win;
}

static SDL_GLContext wrap_SDL_GL_CreateContext(SDL_Window *win) {
  // ensure we have proper GL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GLContext ctx = SDL_GL_CreateContext(win);
  printf("SDL_GL_CreateContext(%p) -> %p\n", win, ctx);
  if (!ctx) printf(" - failed: %s\n", SDL_GetError());
  return ctx;
}

static const char *(*real_glGetString)(u32) = NULL;
static const char *wrap_glGetString(u32 id) {
  if (id == 0x1F02)
    return "4.6"; // return nonsense so that glad loads every available function
  if (real_glGetString)
    return real_glGetString(id);
  return NULL;
}

static void (*real_glShaderSource)(u32, int, char **, int *);
static void wrap_glShaderSource(u32 shader, int num, char **strs, int *lens) {
  if (!strs) return;
  // patch #version to say "430     " instead of "4*0 core", for XOpenGLDrv support
  for (int i = 0; i < num; ++i) {
    char *p = strstr(strs[i], "#version 4");
    if (p) memcpy(p + 10, "30     ", 7);
  }
  if (real_glShaderSource)
    real_glShaderSource(shader, num, strs, lens);
}

static void *wrap_SDL_GL_GetProcAddress(const char *name) {
  if (!strcmp(name, "glGetString")) {
    printf("patching glGetString\n");
    real_glGetString = SDL_GL_GetProcAddress(name);
    return wrap_glGetString;
  } else if (!strcmp(name, "glShaderSource")) {
    printf("patching glShaderSource\n");
    real_glShaderSource = SDL_GL_GetProcAddress(name);
    return wrap_glShaderSource;
  }

  void *p = SDL_GL_GetProcAddress(name);
  if (p) return p;

  static const char *exts[] = { "ARB", "EXT "};
  char tmp[128];
  for (size_t i = 0; i < sizeof(exts) / sizeof(*exts); ++i) {
    snprintf(tmp, sizeof(tmp), "%s%s", name, exts[i]);
    p = SDL_GL_GetProcAddress(tmp);
    if (p) return p;
  }

  printf("SDL_GL_GetProcAddress(): `%s` not found\n", name);

  return NULL;
}

static SDL_bool wrap_SDL_GL_ExtensionSupported(const char *name) {
  if (strstr(name, "shader_draw_parameters"))
    return SDL_FALSE; // not properly supported despite being in ext list
  if (strstr(name, "texture_storage"))
    return SDL_TRUE;  // listed as ARB_ instead of EXT_
  if (strstr(name, "bindless_texture"))
    return SDL_FALSE;  // listed as ARB_ instead of IMG_
  if (strstr(name, "texture_compression_s3tc"))
    return SDL_TRUE;  // for some reason was reported as unsupported
  if (strstr(name, "texture_filter_anisotropic"))
    return SDL_TRUE;  // for some reason was reported as unsupported
  return SDL_GL_ExtensionSupported(name);
}

// the game likes executing this on startup, but on the switch it also pops up the OSK
static void wrap_SDL_StartTextInput(void) { }

// the game uses the SDL_Joystick API, but SDL_GameController is way better
static int wrap_SDL_Init(u32 flags) {
  const int ret = SDL_Init(flags | SDL_INIT_GAMECONTROLLER);
  SDL_GameControllerEventState(SDL_IGNORE);
  SDL_JoystickEventState(SDL_IGNORE);
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
  if (SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt") < 0)
    SDL_GameControllerAddMappingsFromFile("../gamecontrollerdb.txt"); // might be a dir up from SystemARM
  return ret;
}

static const u8 *wrap_SDL_GetKeyboardState(int *numkeys) {
  if (numkeys)
    *numkeys = SDL_NUM_SCANCODES;
  return sdl_keys;
}

static SDL_Joystick *wrap_SDL_JoystickOpen(int index) {
  sdl_ctrl = SDL_GameControllerOpen(index);
  if (sdl_ctrl)
    return SDL_GameControllerGetJoystick(sdl_ctrl);
  return NULL;
}

static void wrap_SDL_JoystickClose(SDL_Joystick *joy) {
  if (sdl_ctrl) {
    SDL_GameControllerClose(sdl_ctrl);
    sdl_ctrl = NULL;
  }
}

static inline void do_osk(void) {
  SwkbdConfig kbd;
  char out[1024];
  Result rc;
  out[0] = 0;
  out[sizeof(out) - 1] = 0;

  rc = swkbdCreate(&kbd, 0);
  if (R_FAILED(rc)) return;

  swkbdConfigMakePresetDefault(&kbd);
  rc = swkbdShow(&kbd, out, sizeof(out));
  swkbdClose(&kbd);
  if (R_FAILED(rc)) return;

  static SDL_Event ev;
  ev.text.type = ev.type = SDL_TEXTINPUT;
  ev.text.timestamp = SDL_GetTicks();
  for (char *p = out; *p; ++p) {
    ev.text.text[0] = *p;
    SDL_PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
  }
}

// we'll map controller buttons to keys and get axis values from the GameController system

static inline void check_btn(const int val, const int map) {
  static SDL_Event ev;
  const int new_btn = val;
  const int old_btn = sdl_keys[map];
  // need to inject keypress events for the UI to work
  if (new_btn && !old_btn) {
    ev.key.type = ev.type = SDL_KEYDOWN;
    ev.key.timestamp = SDL_GetTicks();
    ev.key.keysym.scancode = map;
    ev.key.keysym.sym =SDL_SCANCODE_TO_KEYCODE(map);
    ev.key.state = SDL_PRESSED;
    SDL_PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
  } else if (!new_btn && old_btn) {
    ev.key.type = ev.type = SDL_KEYUP;
    ev.key.timestamp = SDL_GetTicks();
    ev.key.keysym.scancode = map;
    ev.key.keysym.sym =SDL_SCANCODE_TO_KEYCODE(map);
    ev.key.state = SDL_RELEASED;
    SDL_PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
  }
  sdl_keys[map] = new_btn;
}

// do all the input processing here since the game calls it right before PollEvent
static void wrap_SDL_JoystickUpdate(void) {
  static const int keymap[] = {
    SDL_SCANCODE_RETURN,    // SDL_CONTROLLER_BUTTON_A
    SDL_SCANCODE_BACKSPACE, // SDL_CONTROLLER_BUTTON_B
    SDL_SCANCODE_SPACE,     // SDL_CONTROLLER_BUTTON_X
    SDL_SCANCODE_DELETE,    // SDL_CONTROLLER_BUTTON_Y
    0,                      // SDL_CONTROLLER_BUTTON_BACK
    0,                      // SDL_CONTROLLER_BUTTON_GUIDE
    SDL_SCANCODE_ESCAPE,    // SDL_CONTROLLER_BUTTON_START
    SDL_SCANCODE_KP_0,      // SDL_CONTROLLER_BUTTON_LSTICK
    SDL_SCANCODE_KP_1,      // SDL_CONTROLLER_BUTTON_RSTICK
    SDL_SCANCODE_LSHIFT,    // SDL_CONTROLLER_BUTTON_LSHOULDER
    SDL_SCANCODE_RSHIFT,    // SDL_CONTROLLER_BUTTON_RSHOULDER
    SDL_SCANCODE_UP,        // SDL_CONTROLLER_BUTTON_DPAD_UP
    SDL_SCANCODE_DOWN,      // SDL_CONTROLLER_BUTTON_DPAD_DOWN
    SDL_SCANCODE_LEFT,      // SDL_CONTROLLER_BUTTON_DPAD_LEFT
    SDL_SCANCODE_RIGHT,     // SDL_CONTROLLER_BUTTON_DPAD_RIGHT
  };

  static const int trigmap[] = {
    SDL_SCANCODE_LCTRL,     // trigger left
    SDL_SCANCODE_RCTRL,     // trigger right
  };

  static int osk_button = 0;

  SDL_GameControllerUpdate();

  for (size_t i = 0; i < sizeof(keymap) / sizeof(*keymap); ++i)
    check_btn(SDL_GameControllerGetButton(sdl_ctrl, i), keymap[i]);

  for (size_t i = 0; i < sizeof(trigmap) / sizeof(*trigmap); ++i) {
    const int val = (SDL_GameControllerGetAxis(sdl_ctrl, SDL_CONTROLLER_AXIS_TRIGGERLEFT + i) > TRIGGER_THRESHOLD);
    check_btn(val, trigmap[i]);
  }

  // bring up the OSK if the user presses MINUS
  const int new_btn = SDL_GameControllerGetButton(sdl_ctrl, SDL_CONTROLLER_BUTTON_BACK);
  if (new_btn && !osk_button)
    do_osk();
  osk_button = new_btn;
}

static int wrap_SDL_JoystickNumButtons(SDL_Joystick *joy) {
  return 0; // our buttons are mapped to keyboard instead
}

static s16 wrap_SDL_JoystickGetAxis(SDL_Joystick *joy, int axis) {
  int val = SDL_GameControllerGetAxis(sdl_ctrl, axis);
  if (abs(val) < AXIS_DEADZONE) val = 0;
  return (axis & 1) ? -val : val; // invert vertical axis
}

// our SDL2 version is too old to have this
void SDL_SetWindowAlwaysOnTop(void *window, int yes) { }

// __libc_start_main replacement
static void start_main(int (*main_fn)(int, char **, char **), int argc, char **argv, ...) {
  printf("calling main() @ %p\n", main_fn);
  main_fn(fake_argc, fake_argv, NULL);
  exit(0);
}

/* tables */

static const solder_export_t aux_exports[] = {
  SOLDER_EXPORT_SYMBOL(__cxa_atexit),
  SOLDER_EXPORT_SYMBOL(_Unwind_Resume),

  SOLDER_EXPORT_SYMBOL(setjmp),
  SOLDER_EXPORT_SYMBOL(longjmp),

  SOLDER_EXPORT_SYMBOL(inet_pton),
  SOLDER_EXPORT_SYMBOL(getifaddrs),

  SOLDER_EXPORT_SYMBOL(setbuf),
  SOLDER_EXPORT_SYMBOL(setvbuf),

  SOLDER_EXPORT_SYMBOL(wcscat),
  SOLDER_EXPORT_SYMBOL(wcscasecmp),
  SOLDER_EXPORT_SYMBOL(wcschr),
  SOLDER_EXPORT_SYMBOL(wcsdup),
  SOLDER_EXPORT_SYMBOL(wcsncat),
  SOLDER_EXPORT_SYMBOL(wcsncasecmp),
  SOLDER_EXPORT_SYMBOL(wcsncmp),
  SOLDER_EXPORT_SYMBOL(wcsncpy),
  SOLDER_EXPORT_SYMBOL(wcsnlen),
  SOLDER_EXPORT_SYMBOL(wcsstr),
  SOLDER_EXPORT_SYMBOL(wcstol),
  SOLDER_EXPORT_SYMBOL(wcstombs),
  SOLDER_EXPORT_SYMBOL(wprintf),
  SOLDER_EXPORT_SYMBOL(vswprintf),

  SOLDER_EXPORT_SYMBOL(strsep),

  SOLDER_EXPORT_SYMBOL(sqrtf),
  SOLDER_EXPORT_SYMBOL(sin),
  SOLDER_EXPORT_SYMBOL(cos),
  SOLDER_EXPORT_SYMBOL(tan),
  SOLDER_EXPORT_SYMBOL(atan2),
  SOLDER_EXPORT_SYMBOL(fmod),

  SOLDER_EXPORT_SYMBOL(access),
  SOLDER_EXPORT_SYMBOL(unlink),

  SOLDER_EXPORT_SYMBOL(signal),
  SOLDER_EXPORT_SYMBOL(sigaction),

  SOLDER_EXPORT_SYMBOL(sleep),
  SOLDER_EXPORT_SYMBOL(usleep),
  SOLDER_EXPORT_SYMBOL(nanosleep),

  SOLDER_EXPORT_SYMBOL(pthread_kill),

  SOLDER_EXPORT_SYMBOL(iconv),
  SOLDER_EXPORT_SYMBOL(iconv_open),
  SOLDER_EXPORT_SYMBOL(iconv_close),

  SOLDER_EXPORT_SYMBOL(putenv),

  SOLDER_EXPORT_SYMBOL(__h_errno_location),

  SOLDER_EXPORT_SYMBOL(sincos),
  SOLDER_EXPORT_SYMBOL(sincosf),

  SOLDER_EXPORT_SYMBOL(SDL_SetWindowAlwaysOnTop),

  SOLDER_EXPORT_SYMBOL(xmp_set_player),
  SOLDER_EXPORT_SYMBOL(xmp_get_player),
  SOLDER_EXPORT_SYMBOL(xmp_start_player),
  SOLDER_EXPORT_SYMBOL(xmp_end_player),
  SOLDER_EXPORT_SYMBOL(xmp_create_context),
  SOLDER_EXPORT_SYMBOL(xmp_get_module_info),
  SOLDER_EXPORT_SYMBOL(xmp_get_frame_info),
  SOLDER_EXPORT_SYMBOL(xmp_load_module_from_memory),
  SOLDER_EXPORT_SYMBOL(xmp_release_module),
  SOLDER_EXPORT_SYMBOL(xmp_restart_module),
  SOLDER_EXPORT_SYMBOL(xmp_seek_time),
  SOLDER_EXPORT_SYMBOL(xmp_free_context),
  SOLDER_EXPORT_SYMBOL(xmp_set_position),
  SOLDER_EXPORT_SYMBOL(xmp_play_buffer),

  SOLDER_EXPORT_SYMBOL(alcMakeContextCurrent),
  SOLDER_EXPORT_SYMBOL(alcGetContextsDevice),
  SOLDER_EXPORT_SYMBOL(alcOpenDevice),
  SOLDER_EXPORT_SYMBOL(alcGetCurrentContext),
  SOLDER_EXPORT_SYMBOL(alcDestroyContext),
  SOLDER_EXPORT_SYMBOL(alcGetIntegerv),
  SOLDER_EXPORT_SYMBOL(alcGetError),
  SOLDER_EXPORT_SYMBOL(alcGetString),
  SOLDER_EXPORT_SYMBOL(alcCreateContext),
  SOLDER_EXPORT_SYMBOL(alcCloseDevice),
  SOLDER_EXPORT_SYMBOL(alcIsExtensionPresent),

  SOLDER_EXPORT_SYMBOL(alureBufferDataFromMemory),
  SOLDER_EXPORT_SYMBOL(alureShutdownDevice),
  SOLDER_EXPORT_SYMBOL(alureGetErrorString),

  SOLDER_EXPORT_SYMBOL(alDeleteBuffers),
  SOLDER_EXPORT_SYMBOL(alGenSources),
  SOLDER_EXPORT_SYMBOL(alGetError),
  SOLDER_EXPORT_SYMBOL(alDistanceModel),
  SOLDER_EXPORT_SYMBOL(alSourceQueueBuffers),
  SOLDER_EXPORT_SYMBOL(alFilteri),
  SOLDER_EXPORT_SYMBOL(alSourcei),
  SOLDER_EXPORT_SYMBOL(alIsBuffer),
  SOLDER_EXPORT_SYMBOL(alDopplerFactor),
  SOLDER_EXPORT_SYMBOL(alGetSourcei),
  SOLDER_EXPORT_SYMBOL(alFilterf),
  SOLDER_EXPORT_SYMBOL(alDeleteSources),
  SOLDER_EXPORT_SYMBOL(alSource3f),
  SOLDER_EXPORT_SYMBOL(alSourceUnqueueBuffers),
  SOLDER_EXPORT_SYMBOL(alSourcePlay),
  SOLDER_EXPORT_SYMBOL(alIsSource),
  SOLDER_EXPORT_SYMBOL(alListenerfv),
  SOLDER_EXPORT_SYMBOL(alSpeedOfSound),
  SOLDER_EXPORT_SYMBOL(alSource3i),
  SOLDER_EXPORT_SYMBOL(alGetString),
  SOLDER_EXPORT_SYMBOL(alBufferData),
  SOLDER_EXPORT_SYMBOL(alGenBuffers),
  SOLDER_EXPORT_SYMBOL(alSourcef),
  SOLDER_EXPORT_SYMBOL(alGetProcAddress),
  SOLDER_EXPORT_SYMBOL(alSourcefv),
  SOLDER_EXPORT_SYMBOL(alGetBufferi),
  SOLDER_EXPORT_SYMBOL(alSourceStop),
  SOLDER_EXPORT_SYMBOL(alListenerf),
  SOLDER_EXPORT_SYMBOL(alGetSourcef),

  SOLDER_EXPORT("stdin", &export_stdin),
  SOLDER_EXPORT("stdout", &export_stdout),
  SOLDER_EXPORT("stderr", &export_stderr),

  SOLDER_EXPORT("bcmp", memcmp),
  SOLDER_EXPORT("_setjmp", setjmp),
  SOLDER_EXPORT("_longjmp", longjmp),
  SOLDER_EXPORT("__errno_location", __errno),
  SOLDER_EXPORT("__pthread_key_create", pthread_key_create),
  SOLDER_EXPORT("sigemptyset", sigemptyset_fn),
  SOLDER_EXPORT("tolower", tolower_fn),
  SOLDER_EXPORT("toupper", toupper_fn),
  SOLDER_EXPORT("__isoc99_sscanf", sscanf),

  SOLDER_EXPORT("dlopen", solder_dlopen),
  SOLDER_EXPORT("dlclose", solder_dlclose),
  SOLDER_EXPORT("dlsym", solder_dlsym),
  SOLDER_EXPORT("dlerror", solder_dlerror),

  SOLDER_EXPORT("setpwent", dummy_fn),
  SOLDER_EXPORT("endpwent", dummy_fn),

  SOLDER_EXPORT("__libc_start_main", start_main),
};

static const solder_export_t override_exports[] = {
  // SOLDER_EXPORT("__cxa_atexit", dummy_fn),
  SOLDER_EXPORT("accept", wrap_accept),
  SOLDER_EXPORT("connect", wrap_connect),
  SOLDER_EXPORT("bind", wrap_bind),
  SOLDER_EXPORT("sendto", wrap_sendto),
  SOLDER_EXPORT("recvfrom", wrap_recvfrom),
  SOLDER_EXPORT("getsockname", wrap_getsockname),
  SOLDER_EXPORT("getsockopt", wrap_getsockopt),
  SOLDER_EXPORT("setsockopt", wrap_setsockopt),
  SOLDER_EXPORT("fcntl", wrap_fcntl),
  SOLDER_EXPORT("read", wrap_read),
  SOLDER_EXPORT("readdir", wrap_readdir),
  SOLDER_EXPORT("clock_gettime", wrap_clock_gettime),
  SOLDER_EXPORT("SDL_Init", wrap_SDL_Init),
  SOLDER_EXPORT("SDL_GL_CreateContext", wrap_SDL_GL_CreateContext),
  SOLDER_EXPORT("SDL_GL_SetAttribute", wrap_SDL_GL_SetAttribute),
  SOLDER_EXPORT("SDL_GL_GetProcAddress", wrap_SDL_GL_GetProcAddress),
  SOLDER_EXPORT("SDL_GL_ExtensionSupported", wrap_SDL_GL_ExtensionSupported),
  SOLDER_EXPORT("SDL_CreateWindow", wrap_SDL_CreateWindow),
  SOLDER_EXPORT("SDL_StartTextInput", wrap_SDL_StartTextInput),
  SOLDER_EXPORT("SDL_GetKeyboardState", wrap_SDL_GetKeyboardState),
  SOLDER_EXPORT("SDL_JoystickOpen", wrap_SDL_JoystickOpen),
  SOLDER_EXPORT("SDL_JoystickClose", wrap_SDL_JoystickClose),
  SOLDER_EXPORT("SDL_JoystickUpdate", wrap_SDL_JoystickUpdate),
  SOLDER_EXPORT("SDL_JoystickGetAxis", wrap_SDL_JoystickGetAxis),
  SOLDER_EXPORT("SDL_JoystickNumButtons", wrap_SDL_JoystickNumButtons),
};

const solder_export_t *__solder_aux_exports = aux_exports;
const size_t __solder_num_aux_exports = sizeof(aux_exports) / sizeof(*aux_exports);

const solder_export_t *__solder_override_exports = override_exports;
const size_t __solder_num_override_exports = sizeof(override_exports) / sizeof(*override_exports);
