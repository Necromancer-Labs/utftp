/*
 * utftp - Packet handling
 */

#ifndef UTFTP_PACKET_H
#define UTFTP_PACKET_H

#include <stdint.h>
#include <stddef.h>
#include "utftp.h"

/* Parse RRQ/WRQ packet */
int packet_parse_request(uint8_t *buf, size_t len, char *filename, size_t fn_len,
                         char *mode, size_t mode_len, size_t *blksize, size_t *tsize);

/* Build packets */
int packet_build_data(uint8_t *buf, uint16_t block, uint8_t *data, size_t data_len);
int packet_build_ack(uint8_t *buf, uint16_t block);
int packet_build_error(uint8_t *buf, tftp_error_t code, const char *msg);
int packet_build_oack(uint8_t *buf, size_t blksize, size_t tsize, int has_tsize);

#endif /* UTFTP_PACKET_H */
