#include "MEM_guardedalloc.h"

#include "NET_socket.h"

#include <sys/socket.h>

/**
 * Not a huge fun of having platform specific defines but I would hate virtual functions here even more.
 * I have decided to manipulate function and declaration from a single platform instead of both.
 */
#ifndef WIN32
#	define SOCKET int
#	define closesocket close
#endif

typedef struct NetSocket {
	SOCKET fd;
} NetSocket;

NetSocket *NET_socket_new(NetAddress *addr) {
	return NET_socket_new_ex(NET_address_family(addr), NET_address_socktype(addr), NET_address_protocol(addr));
}

NetSocket *NET_socket_new_ex(int af, int type, int protocol) {
	NetSocket *sock = MEM_mallocN(sizeof(NetSocket), "NetSocket");
	if ((sock->fd = socket(af, type, protocol)) == (SOCKET)-1) {
		MEM_SAFE_FREE(socket);
	}
	return sock;
}

void NET_socket_free(NetSocket *sock) {
	NET_socket_close(sock);
	MEM_freeN(sock);
}

void NET_socket_close(NetSocket *sock) {
	if (sock->fd != (SOCKET)-1) {
		// shutdown(sock->fd, SHUT_RDWR);
		closesocket(sock->fd); // Not a native Linux/Unix function but defined above.
		sock->fd = (SOCKET)-1;
	}
}

#ifndef WIN32

void NET_init() {
}

void NET_exit() {
}

#else

void NET_init() {
	WSADATA data;

	int ret;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &data)) != 0) {
		abort();
	}
}

void NET_exit() {
	WSACleanup();
}

#endif
