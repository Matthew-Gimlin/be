#include "error.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

noreturn void be_fatal(const char* fmt, ...) {
    fprintf(stderr, "be: error: ");
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(1);
}

void be_error(const char* filename, int line, const char* fmt, ...) {
    fprintf(stderr, "%s:%d: error: ", filename, line);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(1);
}
