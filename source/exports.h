#pragma once

#include <stdio.h>

#define MAX_FAKE_ARGC 20

extern FILE *export_stdin;
extern FILE *export_stdout;
extern FILE *export_stderr;

extern char *fake_argv[MAX_FAKE_ARGC];
extern int fake_argc;
