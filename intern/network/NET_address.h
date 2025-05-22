#ifndef NET_ADDRESS_H
#define NET_ADDRESS_H

#ifndef WIN32
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netdb.h>
#else
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	pragma comment(lib, "Ws2_32.lib")
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque object hiding the NetAddress defined in address.c */
typedef struct NetAddress NetAddress;

struct NetAddress *NET_addresss_new(void);
struct NetAddress *NET_address_new_ex(const char *mode, const char *service, int af, int sock);
void NET_address_free(NetAddress *addr);

/**
 * Provide protocol-independent translation from an ANSI host name to an address.
 *
 * \param addr The address structure we want to resolve the result to.
 * \param mode The null-terminated ANSI string that contains a host (mode) name or a numeric host address string.
 * \param service The null-terminated ANSI string that contains either a service name or a port number represented as a string.
 * \param af The address family. Possible values for the address family are defined in the AF_XXX enum.
 * \param sock The socket type. Possible values for the socket type are defined in the SOCK_XXX enum.
 *
 * \return Success returns zero. Failure return nonzero error code, as found in the NET_error.h error codes.
 */
int NET_address_init(NetAddress *addr, const char *mode, const char *service, int af, int sock);

/**
 * A network address resolved from the #NET_address_init might return mutliple resoved address information.
 * \param addr The address structure we have resolved.
 * \return Success returns zero. Failure (no more entries) return nonzero value.
 *
 * \code{.c}
 * NetAddress *addr = NET_address_new_ex("127.0.0.1", "http", AF_UNSPEC, SOCK_TCP);
 * do {
 * 	...
 * } while(NET_address_next(addr));
 * \endcode
 */
bool NET_address_next(NetAddress *addr);

int NET_address_protocol(const NetAddress *addr);
int NET_address_socktype(const NetAddress *addr);
int NET_address_family(const NetAddress *addr);

#ifdef __cplusplus
}
#endif

#endif
