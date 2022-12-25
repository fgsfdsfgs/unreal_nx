#pragma once

void fatal_error(const char *fmt, ...) __attribute__((noreturn));

int file_exists(const char *fname);

void nxlink_init(void);
void nxlink_quit(void);

void *get_tpidr_el0(void);
void set_tpidr_el0(void *addr);
