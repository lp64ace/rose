#include "MEM_guardedalloc.h"

#include "NET_address.h"

typedef struct NetAddress {
	struct addrinfo *ptr;
	struct addrinfo *itr;
	int index;
} NetAddress;

NetAddress *NET_address_new() {
	NetAddress *addr = MEM_mallocN(sizeof(NetAddress), "NetAddress");
	addr->ptr = NULL;
	addr->itr = NULL;
	addr->index = 0;
	return addr;
}

NetAddress *NET_address_new_ex(const char *mode, const char *service, int af, int sock) {
	NetAddress *addr = NET_address_new();
	if (NET_address_init(addr, mode, service, af, sock) != 0) {
		MEM_SAFE_FREE(addr);
	}
	return addr;
}

int NET_address_init(NetAddress *addr, const char *mode, const char *service, int af, int sock) {
	struct addrinfo hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = af;
	hint.ai_socktype = sock;

	struct addrinfo *resolved;

	int ret = 0;
	if ((ret = getaddrinfo(mode, service, &hint, &resolved)) != 0) {
		return ret;
	}

	addr->ptr = resolved;
	addr->itr = addr->ptr;
	addr->index = 0;

	return ret;
}

void NET_address_clear(NetAddress *addr) {
	if (addr->ptr) {
		freeaddrinfo(addr->ptr);
	}
	addr->ptr = NULL;
	addr->itr = NULL;
	addr->index = 0;
}

void NET_address_free(NetAddress *addr) {
	NET_address_clear(addr);
	MEM_freeN(addr);
}

bool NET_address_next(NetAddress *addr) {
	if (addr->itr != NULL && addr->itr->ai_next != NULL) {
		addr->itr = addr->itr->ai_next;
	}
	else {
		addr->itr = addr->ptr;
	}
	return (addr->itr == addr->ptr);
}

const char *NET_address_name(const NetAddress *addr) {
	return (addr->itr) ? addr->itr->ai_canonname : NULL;
}

int NET_address_family(const NetAddress *addr) {
	return (addr->itr) ? addr->itr->ai_family : -1;
}
int NET_address_socktype(const NetAddress *addr) {
	return (addr->itr) ? addr->itr->ai_socktype : -1;
}
int NET_address_protocol(const NetAddress *addr) {
	return (addr->itr) ? addr->itr->ai_protocol : -1;
}

