#include "MEM_guardedalloc.h"

#include "NET_socket.h"

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
		MEM_SAFE_FREE(sock);
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

int NET_socket_bind(NetSocket *sock, NetAddressIn *addr_in) {
	return bind(sock->fd, NET_address_in_data(addr_in), NET_address_in_length(addr_in));
}
int NET_socket_connect(NetSocket *sock, NetAddress *addr) {
	return connect(sock->fd, NET_address_data(addr), NET_address_length(addr));
}
int NET_socket_listen(NetSocket *sock) {
	return NET_socket_listen_ex(sock, SOMAXCONN);
}
int NET_socket_listen_ex(NetSocket *sock, int backlog) {
	return listen(sock->fd, backlog);
}

bool NET_socket_select(NetSocket *sock, long long usec) {
	fd_set m;
	FD_ZERO(&m);
	FD_SET(sock->fd, &m);

	struct timeval t;
	t.tv_sec = usec / 1000000;
	t.tv_usec = usec % 1000000;

	int ready = select(sock->fd + 1, &m, NULL, NULL, &t);
	if (ready > 0 && FD_ISSET(sock->fd, &m)) {
		return true;
	}
	return false;
}

NetSocket *NET_socket_accept(NetSocket *server) {
	NetSocket *sock = MEM_mallocN(sizeof(NetSocket), "NetSocket");
	if ((sock->fd = accept(server->fd, NULL, NULL)) == (SOCKET)-1) {
		MEM_SAFE_FREE(sock);
	}

	return sock;
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
