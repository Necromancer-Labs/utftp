/*
 * utftp - Server core
 */

#ifndef UTFTP_SERVER_H
#define UTFTP_SERVER_H

#include "utftp.h"

/* Server lifecycle */
int  tftp_server_init(tftp_server_t *srv, tftp_config_t *config);
int  tftp_server_run(tftp_server_t *srv);
void tftp_server_stop(tftp_server_t *srv);
void tftp_server_cleanup(tftp_server_t *srv);

#endif /* UTFTP_SERVER_H */
