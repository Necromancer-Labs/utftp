/*
 * utftp - Utility functions
 */

#ifndef UTFTP_UTIL_H
#define UTFTP_UTIL_H

#include <stddef.h>

/* Path security */
int validate_path(const char *root, const char *filename, char *fullpath, size_t pathlen);

/* Formatting */
const char* format_size(size_t bytes, char *buf, size_t buflen);
const char* format_speed(double bytes_per_sec, char *buf, size_t buflen);

#endif /* UTFTP_UTIL_H */
