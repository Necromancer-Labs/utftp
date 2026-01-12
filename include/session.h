/*
 * utftp - Session management
 */

#ifndef UTFTP_SESSION_H
#define UTFTP_SESSION_H

#include "utftp.h"

/* Session lifecycle */
tftp_session_t* session_alloc(tftp_server_t *srv);
void session_free(tftp_session_t *sess);
tftp_session_t* session_find_by_addr(tftp_server_t *srv, struct sockaddr_in *addr);

/* Session socket */
int session_create_socket(tftp_server_t *srv);

/* Packet I/O */
int session_send_packet(tftp_session_t *sess, uint8_t *buf, size_t len);
int session_retransmit(tftp_session_t *sess);
void session_send_error(tftp_session_t *sess, tftp_error_t code, const char *msg);

#endif /* UTFTP_SESSION_H */
