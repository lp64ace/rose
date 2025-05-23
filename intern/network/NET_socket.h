#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "NET_address.h"
#include "NET_bind.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque object hiding the NetSocket defined in socket.c */
typedef struct NetSocket NetSocket;

struct NetSocket *NET_socket_new(NetAddress *addr);
struct NetSocket *NET_socket_new_ex(int af, int socktype, int protocol);

void NET_socket_close(NetSocket *socket);
void NET_socket_free(NetSocket *socket);

int NET_socket_bind(NetSocket *socket, NetAddressIn *addr_in);
int NET_socket_connect(NetSocket *socket, NetAddress *addr);

int NET_socket_listen(NetSocket *socket);
/** The listen function places a socket in a state in which it is listening for an incoming connection. */
int NET_socket_listen_ex(NetSocket *socket, int backlog);

bool NET_socket_select(NetSocket *socket, long long usec);
struct NetSocket *NET_socket_accept(NetSocket *socket);

size_t NET_socket_send_ex(NetSocket *socket, const void *data, size_t length, int flag);
size_t NET_socket_recv_ex(NetSocket *socket, void *data, size_t length, int flag);
size_t NET_socket_send(NetSocket *socket, const void *data, size_t length);
size_t NET_socket_recv(NetSocket *socket, void *data, size_t length);
size_t NET_socket_peek(NetSocket *socket, void *data, size_t length);
bool NET_socket_valid(NetSocket *socket);

void NET_init();
void NET_exit();

#ifdef __cplusplus
}
#endif

#endif
