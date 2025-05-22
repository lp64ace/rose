#include "NET_socket.h"

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
