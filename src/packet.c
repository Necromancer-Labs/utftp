/*
 * utftp - Packet handling
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "../include/packet.h"

int packet_parse_request(uint8_t *buf, size_t len, char *filename, size_t fn_len,
                         char *mode, size_t mode_len, size_t *blksize, size_t *tsize)
{
    if (len < 4)
        return -1;

    /* Skip opcode (already read) */
    uint8_t *p = buf + 2;
    uint8_t *end = buf + len;

    /* Extract filename (null-terminated) */
    char *fn_start = (char *)p;
    while (p < end && *p != '\0') p++;
    if (p >= end) return -1;

    size_t fn_size = p - (uint8_t *)fn_start;
    if (fn_size >= fn_len) fn_size = fn_len - 1;
    memcpy(filename, fn_start, fn_size);
    filename[fn_size] = '\0';
    p++;

    /* Extract mode (null-terminated) */
    char *mode_start = (char *)p;
    while (p < end && *p != '\0') p++;
    if (p >= end) return -1;

    size_t mode_size = p - (uint8_t *)mode_start;
    if (mode_size >= mode_len) mode_size = mode_len - 1;
    memcpy(mode, mode_start, mode_size);
    mode[mode_size] = '\0';
    p++;

    /* Default values */
    *blksize = TFTP_DEF_BLKSIZE;
    *tsize = 0;

    /* Parse options */
    while (p < end) {
        char *opt_name = (char *)p;
        while (p < end && *p != '\0') p++;
        if (p >= end) break;
        p++;

        char *opt_val = (char *)p;
        while (p < end && *p != '\0') p++;
        if (p >= end) break;
        p++;

        if (strcasecmp(opt_name, "blksize") == 0) {
            size_t bs = strtoul(opt_val, NULL, 10);
            if (bs >= TFTP_MIN_BLKSIZE && bs <= TFTP_MAX_BLKSIZE) {
                *blksize = bs;
            }
        } else if (strcasecmp(opt_name, "tsize") == 0) {
            *tsize = strtoul(opt_val, NULL, 10);
        }
    }

    return 0;
}

int packet_build_data(uint8_t *buf, uint16_t block, uint8_t *data, size_t data_len)
{
    buf[0] = 0;
    buf[1] = TFTP_DATA;
    buf[2] = (block >> 8) & 0xFF;
    buf[3] = block & 0xFF;
    if (data_len > 0)
        memcpy(buf + 4, data, data_len);
    return 4 + data_len;
}

int packet_build_ack(uint8_t *buf, uint16_t block)
{
    buf[0] = 0;
    buf[1] = TFTP_ACK;
    buf[2] = (block >> 8) & 0xFF;
    buf[3] = block & 0xFF;
    return 4;
}

int packet_build_error(uint8_t *buf, tftp_error_t code, const char *msg)
{
    buf[0] = 0;
    buf[1] = TFTP_ERROR;
    buf[2] = (code >> 8) & 0xFF;
    buf[3] = code & 0xFF;

    size_t msg_len = strlen(msg);
    memcpy(buf + 4, msg, msg_len);
    buf[4 + msg_len] = '\0';
    return 5 + msg_len;
}

int packet_build_oack(uint8_t *buf, size_t blksize, size_t tsize, int has_tsize)
{
    buf[0] = 0;
    buf[1] = TFTP_OACK;
    int offset = 2;

    if (blksize != TFTP_DEF_BLKSIZE) {
        offset += sprintf((char *)buf + offset, "blksize") + 1;
        offset += sprintf((char *)buf + offset, "%zu", blksize) + 1;
    }

    if (has_tsize) {
        offset += sprintf((char *)buf + offset, "tsize") + 1;
        offset += sprintf((char *)buf + offset, "%zu", tsize) + 1;
    }

    return offset;
}
