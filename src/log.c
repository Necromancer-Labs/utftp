/*
 * utftp - Logging implementation
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "../include/log.h"

/* Global state */
int g_log_level = LOG_INFO;
int g_use_color = 1;

void log_msg(int level, const char *fmt, ...)
{
    if (level < g_log_level)
        return;

    const char *prefix;
    const char *color;
    FILE *out = stdout;

    switch (level) {
        case LOG_DEBUG:    prefix = "DBG"; color = C_DIM;     break;
        case LOG_INFO:     prefix = "INF"; color = C_CYAN;    break;
        case LOG_WARN:     prefix = "WRN"; color = C_YELLOW;  break;
        case LOG_ERROR:    prefix = "ERR"; color = C_RED;     out = stderr; break;
        case LOG_CRITICAL: prefix = "CRT"; color = C_RED C_BOLD; out = stderr; break;
        default:           prefix = "   "; color = "";
    }

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm);

    if (g_use_color) {
        fprintf(out, "%s%s%s %s[%s]%s ", C_DIM, timebuf, C_RESET, color, prefix, C_RESET);
    } else {
        fprintf(out, "%s [%s] ", timebuf, prefix);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
    fflush(out);
}

void print_banner(void)
{
    if (g_use_color) {
        printf("\n");
        printf("  %s█▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀█%s\n", C_CYAN, C_RESET);
        printf("  %s█%s        Ultra TFTP         %s█%s\n", C_CYAN, C_RESET, C_CYAN, C_RESET);
        printf("  %s█%s       Fast. Simple.       %s█%s\n", C_CYAN, C_RESET, C_CYAN, C_RESET);
        printf("  %s█▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█%s\n", C_CYAN, C_RESET);
        printf("\n");
    } else {
        printf("\n  === Ultra TFTP Server ===\n\n");
    }
}
