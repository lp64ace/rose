#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "NET_address.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque object hiding the NetSocket defined in socket.c */
typedef struct NetSocket NetSocket;

void NET_init();
void NET_exit();

#ifdef __cplusplus
}
#endif

#endif
