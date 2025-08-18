/* first_pass.h - the first pass */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "types.h"
#include <stdarg.h>

/* first pass functions */
void first_pass(const char *filename);
void error(int line_number, const char *format, ...);

#endif /* FIRST_PASS_H */
