#include "MEM_guardedalloc.h"

#include "NET_bind.h"

typedef struct NetAddressIn {
	union {
		struct sockaddr_in a4;
		struct sockaddr_in6 a6;
	};
} NetAddressIn;

NetAddressIn *NET_address_in_new(void) {
	NetAddressIn *addr = MEM_mallocN(sizeof(NetAddressIn), "NetAddressIn");
	if (addr) {
		memset(addr, 0, sizeof(NetAddressIn));
		// *(uint32_t *)&addr->a.sin_addr = htonl(INADDR_ANY);
	}
	return addr;
}
NetAddressIn *NET_address_in_new_ex(const char *address, uint16_t port, int af) {
	NetAddressIn *addr = NET_address_in_new();
	if (addr) {
		if (NET_address_in_init(addr, address, port, af) != 0) {
			MEM_SAFE_FREE(addr);
		}
	}
	return addr;
}
void NET_address_in_free(NetAddressIn *addr) {
	MEM_freeN(addr);
}

int NET_address_in_init(NetAddressIn *addr, const char *address, uint16_t port, int af) {
	switch (af) {
		case AF_INET6: {
			memset(addr, 0, sizeof(NetAddressIn));
			addr->a6.sin6_family = af;
			addr->a6.sin6_port = htons(port);
			return inet_pton(af, address, &addr->a6.sin6_addr) == 0;
		} break;
		default: {
			memset(addr, 0, sizeof(NetAddressIn));
			addr->a4.sin_family = af;
			addr->a4.sin_port = htons(port);
			return inet_pton(af, address, &addr->a4.sin_addr) == 0;
		} break;
	}
	return -1;
}

int NET_address_in_length(const NetAddressIn *addr) {
	if (addr->a6.sin6_family == AF_INET6) {
		return sizeof(addr->a6);
	}
	return sizeof(addr->a4);
}
const struct sockaddr *NET_address_in_data(const NetAddressIn *addr) {
	if (addr->a6.sin6_family == AF_INET6) {
		return (const struct sockaddr *)&addr->a6;
	}
	return (const struct sockaddr *)&addr->a4;
}
