#pragma once

void nxlink_init(void);
void nxlink_quit(void);

void fatal_error(const char *fmt, ...) __attribute__((noreturn));

void *get_tpidr_el0(void);
void set_tpidr_el0(void *addr);
