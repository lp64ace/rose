#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "NET_address.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque object hiding the NetSocket defined in socket.c */
typedef struct NetSocket NetSocket;

/** This method will open the socket and attempt to connect to the specified address. */
NetSocket *NET_socket_new(NetAddress *addr);
NetSocket *NET_socket_new_ex(int af, int socktype, int protocol);

void NET_socket_close(NetSocket *socket);
void NET_socket_free(NetSocket *socket);

void NET_init();
void NET_exit();

#ifdef __cplusplus
}
#endif

#endif
