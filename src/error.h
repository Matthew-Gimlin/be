#ifndef BE_ERROR_H
#define BE_ERROR_H

#include <stdnoreturn.h>

noreturn void be_fatal(const char* fmt, ...);
void be_error(const char* filename, int line, const char* fmt, ...);

#endif
