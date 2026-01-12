/*
 * utftp - Utility functions
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "../include/util.h"
#include "../include/log.h"

int validate_path(const char *root, const char *filename, char *fullpath, size_t pathlen)
{
    char resolved_root[PATH_MAX];
    char resolved_path[PATH_MAX];
    char temp_path[PATH_MAX];

    /* Resolve root directory */
    if (!realpath(root, resolved_root)) {
        log_msg(LOG_ERROR, "Cannot resolve root path: %s", root);
        return -1;
    }

    /* Build full path */
    snprintf(temp_path, sizeof(temp_path), "%s/%s", resolved_root, filename);

    /* For reading: resolve the full path */
    if (realpath(temp_path, resolved_path)) {
        /* Path exists - verify it's under root */
        size_t root_len = strlen(resolved_root);
        if (strncmp(resolved_path, resolved_root, root_len) != 0 ||
            (resolved_path[root_len] != '/' && resolved_path[root_len] != '\0')) {
            log_msg(LOG_WARN, "Path traversal attempt blocked: %s", filename);
            return -1;
        }
        strncpy(fullpath, resolved_path, pathlen - 1);
        fullpath[pathlen - 1] = '\0';
        return 0;
    }

    /* Path doesn't exist - could be for writing. Verify parent. */
    if (strstr(filename, "..") != NULL) {
        log_msg(LOG_WARN, "Path traversal attempt blocked: %s", filename);
        return -1;
    }

    /* Use the concatenated path for new files */
    strncpy(fullpath, temp_path, pathlen - 1);
    fullpath[pathlen - 1] = '\0';
    return 0;
}

const char* format_size(size_t bytes, char *buf, size_t buflen)
{
    if (bytes < 1024)
        snprintf(buf, buflen, "%zu B", bytes);
    else if (bytes < 1024 * 1024)
        snprintf(buf, buflen, "%.1f KB", bytes / 1024.0);
    else if (bytes < 1024 * 1024 * 1024)
        snprintf(buf, buflen, "%.1f MB", bytes / (1024.0 * 1024.0));
    else
        snprintf(buf, buflen, "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    return buf;
}

const char* format_speed(double bytes_per_sec, char *buf, size_t buflen)
{
    if (bytes_per_sec < 1024)
        snprintf(buf, buflen, "%.0f B/s", bytes_per_sec);
    else if (bytes_per_sec < 1024 * 1024)
        snprintf(buf, buflen, "%.1f KB/s", bytes_per_sec / 1024.0);
    else
        snprintf(buf, buflen, "%.1f MB/s", bytes_per_sec / (1024.0 * 1024.0));
    return buf;
}
