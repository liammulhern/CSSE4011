#include <string.h>
#include <stdlib.h>
#include <parser.h>

/**
 * @brief  Extract a number after a JSON key.
 *
 * @param str      Pointer to the start of the key in a NUL-terminated string,
 *                 e.g. where *str == '"' for "\"x_pos\":"
 * @param key      The key including quotes and colon, e.g. "\"x_pos\":"
 * @param out_val  Pointer to the double to write.
 *
 * @return true on success (key found and number parsed), false otherwise.
 */
bool extract_double(const char *str, const char *key, double *out_val)
{
    const char *p = strstr(str, key);
    if (!p) {
        return false;
    }
    p += strlen(key);           /* jump past the key itself */
    *out_val = strtod(p, NULL); /* parse floating-point */
    return true;
}

/**
 * @brief  Extract a number after a JSON key.
 *
 * Same as extract_double(), but for integers.
 */
bool extract_int(const char *str, const char *key, int *out_val)
{
    const char *p = strstr(str, key);
    if (!p) {
        return false;
    }
    p += strlen(key);
    *out_val = strtol(p, NULL, 10);
    return true;
}
