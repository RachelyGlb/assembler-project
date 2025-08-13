#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "assembler.h"

#include <stdarg.h>

void first_pass(const char *filename);
void error(int line_number, const char *format, ...);

#endif
