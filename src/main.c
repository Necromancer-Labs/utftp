/*
 * utftp - Ultra TFTP Server
 * Entry point and CLI
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include "../include/utftp.h"
#include "../include/server.h"
#include "../include/log.h"

/* Global server pointer for signal handler */
static tftp_server_t *g_server = NULL;

static void signal_handler(int sig)
{
    (void)sig;
    if (g_server) {
        printf("\n");
        log_msg(LOG_INFO, "Shutting down gracefully...");
        tftp_server_stop(g_server);
    }
}

static void print_usage(const char *prog)
{
    printf("Ultra TFTP Server\n\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -i, --ip ADDR       Bind to specific IP address (default: 0.0.0.0)\n");
    printf("  -p, --port PORT     Listen port (default: 69)\n");
    printf("  -r, --root DIR      Root directory (default: current)\n");
    printf("  -t, --timeout SEC   Timeout in seconds (default: 30)\n");
    printf("  -d, --debug         Enable debug logging\n");
    printf("  -q, --quiet         Quiet mode (critical errors only)\n");
    printf("  -h, --help          Show this help\n");
}

int main(int argc, char *argv[])
{
    tftp_config_t config;
    memset(&config, 0, sizeof(config));

    /* Defaults */
    config.port = TFTP_PORT;
    config.timeout_sec = TFTP_TIMEOUT_SEC;
    getcwd(config.root_dir, sizeof(config.root_dir));

    static struct option long_opts[] = {
        {"ip",      required_argument, 0, 'i'},
        {"port",    required_argument, 0, 'p'},
        {"root",    required_argument, 0, 'r'},
        {"timeout", required_argument, 0, 't'},
        {"debug",   no_argument,       0, 'd'},
        {"quiet",   no_argument,       0, 'q'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:p:r:t:dqh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'i':
                strncpy(config.bind_addr, optarg, sizeof(config.bind_addr) - 1);
                break;
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'r':
                if (!realpath(optarg, config.root_dir)) {
                    fprintf(stderr, "Invalid root directory: %s\n", optarg);
                    return 1;
                }
                break;
            case 't':
                config.timeout_sec = atoi(optarg);
                break;
            case 'd':
                config.debug = 1;
                g_log_level = LOG_DEBUG;
                break;
            case 'q':
                config.quiet = 1;
                g_log_level = LOG_CRITICAL;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    /* Detect if terminal supports colors */
    g_use_color = isatty(STDOUT_FILENO);

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    /* Initialize and run server */
    tftp_server_t server;
    g_server = &server;

    if (tftp_server_init(&server, &config) < 0) {
        return 1;
    }

    tftp_server_run(&server);
    tftp_server_cleanup(&server);

    return 0;
}
