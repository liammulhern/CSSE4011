#ifndef PARSER_H_
#define PARSER_H_

#include <stdbool.h>

bool extract_double(const char *str, const char *key, double *out_val);

bool extract_int(const char *str, const char *key, int *out_val);

#endif // PARSER_H_
