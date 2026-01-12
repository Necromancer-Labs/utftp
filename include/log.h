/*
 * utftp - Logging
 */

#ifndef UTFTP_LOG_H
#define UTFTP_LOG_H

/* Log levels */
#define LOG_DEBUG    0
#define LOG_INFO     1
#define LOG_WARN     2
#define LOG_ERROR    3
#define LOG_CRITICAL 4

/* ANSI color codes */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"

/* Global state */
extern int g_log_level;
extern int g_use_color;

/* Functions */
void log_msg(int level, const char *fmt, ...);
void print_banner(void);

#endif /* UTFTP_LOG_H */
