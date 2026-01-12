/*
 * utftp - Transfer handlers
 */

#ifndef UTFTP_TRANSFER_H
#define UTFTP_TRANSFER_H

#include <stdint.h>
#include <stddef.h>
#include "utftp.h"

/* Request handlers */
int handle_rrq(tftp_server_t *srv, tftp_session_t *sess, uint8_t *buf, size_t len);
int handle_wrq(tftp_server_t *srv, tftp_session_t *sess, uint8_t *buf, size_t len);

/* Packet handlers */
int handle_ack(tftp_session_t *sess, uint8_t *buf, size_t len);
int handle_data(tftp_session_t *sess, uint8_t *buf, size_t len);

/* Main packet processor */
int process_session_packet(tftp_session_t *sess, uint8_t *buf, size_t len);

#endif /* UTFTP_TRANSFER_H */
