#ifndef NET_BIND_H
#define NET_BIND_H

#include "NET_address.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NetAddressIn NetAddressIn;

struct NetAddressIn *NET_address_in_new(void);
struct NetAddressIn *NET_address_in_new_ex(const char *address, uint16_t port, int af);
void NET_address_in_free(NetAddressIn *addr);

/**
 * Translate an IPv4 or IPv6 internet network address in its standard text presentation form into its numeric binary form.
 *
 * \param addr The address structure we want to resolve the result to.
 * \param address The null-terminated ANSI string that contains a numeric IPv4 or IPv6 address.
 * \param port The port number to assign to the address.
 * \param af The address family. Possible values for the address family are defined in the AF_XXX enum.
 *
 * \return Success returns zero. Failure return nonzero error code, as found in the NET_error.h error codes.
 */
int NET_address_in_init(NetAddressIn *addr, const char *address, uint16_t port, int af);

int NET_address_in_length(const NetAddressIn *addr);
const struct sockaddr *NET_address_in_data(const NetAddressIn *addr);

#ifdef __cplusplus
}
#endif

#endif