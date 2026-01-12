/*
 * utftp - Ultra TFTP Server
 * Main header with types and constants
 */

#ifndef UTFTP_H
#define UTFTP_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>
#include <sys/time.h>

/* TFTP Constants */
#define TFTP_PORT           69
#define TFTP_DEF_BLKSIZE    512
#define TFTP_MIN_BLKSIZE    8
#define TFTP_MAX_BLKSIZE    65464
#define TFTP_MAX_PACKET     (4 + TFTP_MAX_BLKSIZE)
#define TFTP_TIMEOUT_SEC    30
#define TFTP_MAX_RETRIES    3

/* Limits */
#define MAX_SESSIONS        64
#define MAX_PATH_LEN        1024
#define MAX_FILENAME_LEN    256

/* TFTP Opcodes */
typedef enum {
    TFTP_RRQ   = 1,
    TFTP_WRQ   = 2,
    TFTP_DATA  = 3,
    TFTP_ACK   = 4,
    TFTP_ERROR = 5,
    TFTP_OACK  = 6
} tftp_opcode_t;

/* TFTP Error Codes */
typedef enum {
    TFTP_ERR_UNDEFINED      = 0,
    TFTP_ERR_FILE_NOT_FOUND = 1,
    TFTP_ERR_ACCESS_DENIED  = 2,
    TFTP_ERR_DISK_FULL      = 3,
    TFTP_ERR_ILLEGAL_OP     = 4,
    TFTP_ERR_UNKNOWN_TID    = 5,
    TFTP_ERR_FILE_EXISTS    = 6,
    TFTP_ERR_NO_USER        = 7,
    TFTP_ERR_BAD_OPTIONS    = 8
} tftp_error_t;

/* Session state */
typedef enum {
    STATE_FREE = 0,
    STATE_RRQ_RECV,
    STATE_WRQ_RECV,
    STATE_SENDING,
    STATE_RECEIVING,
    STATE_LAST_DATA,
    STATE_ERROR
} session_state_t;

/* Forward declaration */
typedef struct tftp_server tftp_server_t;

/* Transfer session */
typedef struct {
    session_state_t state;
    int             sock;
    struct sockaddr_in client_addr;

    int             fd;
    char            filename[MAX_FILENAME_LEN];

    uint16_t        block_num;
    size_t          blksize;
    size_t          tsize;
    size_t          bytes_transferred;

    struct timeval  last_activity;
    struct timeval  start_time;
    int             retries;

    uint8_t         last_packet[TFTP_MAX_PACKET];
    size_t          last_packet_len;
} tftp_session_t;

/* Server configuration */
typedef struct {
    char            root_dir[MAX_PATH_LEN];
    char            bind_addr[64];
    uint16_t        port;
    int             timeout_sec;
    int             debug;
    int             quiet;
} tftp_config_t;

/* Server state */
struct tftp_server {
    int             main_sock;
    tftp_config_t   config;
    tftp_session_t  sessions[MAX_SESSIONS];
    volatile int    running;
};

#endif /* UTFTP_H */
