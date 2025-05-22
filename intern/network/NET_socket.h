#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "NET_address.h"
#include "NET_bind.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque object hiding the NetSocket defined in socket.c */
typedef struct NetSocket NetSocket;

NetSocket *NET_socket_new(NetAddress *addr);
NetSocket *NET_socket_new_ex(int af, int socktype, int protocol);

void NET_socket_close(NetSocket *socket);
void NET_socket_free(NetSocket *socket);

int NET_socket_bind(NetSocket *socket, NetAddressIn *addr_in);
int NET_socket_connect(NetSocket *socket, NetAddress *addr);

int NET_socket_listen(NetSocket *socket);
/** The listen function places a socket in a state in which it is listening for an incoming connection. */
int NET_socket_listen_ex(NetSocket *socket, int backlog);

void NET_init();
void NET_exit();

#ifdef __cplusplus
}
#endif

#endif
