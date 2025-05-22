#include "NET_socket.h"

#include "gtest/gtest.h"

namespace {

TEST(Socket, Open) {
	NET_init();
	NetAddress *addr = NET_address_new_ex("127.0.0.1", "6969", AF_INET, SOCK_STREAM);
	ASSERT_NE(addr, nullptr);

	int length = 0;
	do {
		NetSocket *sock = NET_socket_new_ex(NET_address_family(addr), NET_address_socktype(addr), NET_address_protocol(addr));
		if(sock) {
			length++;
			NET_socket_free(sock);
		}
	} while(NET_address_next(addr));

	EXPECT_GT(length, 0);  // At least one socket should open.

	NET_address_free(addr);
	NET_exit();
}

TEST(Socket, Connect) {
	NET_init();
	NetAddress *addr = NET_address_new_ex("google.com", "http", AF_INET, SOCK_STREAM);
	ASSERT_NE(addr, nullptr);

	int length = 0;
	do {
		NetSocket *sock = NET_socket_new(addr);
		if (!sock) {
			continue;
		}

		if (NET_socket_connect(sock, addr) == 0) {
			length++;
		}

		NET_socket_free(sock);
	} while (NET_address_next(addr));

	EXPECT_GT(length, 0);  // At least one socket should connect.

	NET_address_free(addr);
	NET_exit();
}

TEST(Socket, Bind) {
	NET_init();
	NetAddress *addr = NET_address_new_ex("google.com", "http", AF_INET, SOCK_STREAM);
	ASSERT_NE(addr, nullptr);

	int length = 0;
	do {
		NetAddressIn *addr_in = NET_address_in_new_ex("0.0.0.0", 0, NET_address_family(addr));
		if (!addr_in) {
			continue;
		}

		NetSocket *sock = NET_socket_new(addr);
		if (sock) {
			do {
				if (NET_socket_bind(sock, addr_in) != 0) {
					break;
				}
				if (NET_socket_connect(sock, addr) != 0) {
					break;
				}
				length++;
			} while (false);

			NET_socket_free(sock);
		}

		NET_address_in_free(addr_in);
	} while (NET_address_next(addr));

	EXPECT_GT(length, 0);  // At least one socket should connect.

	NET_address_free(addr);
	NET_exit();
}

TEST(Socket, Listen) {
	NET_init();
	NetAddress *addr = NET_address_new_ex("127.0.0.1", "6969", AF_INET, SOCK_STREAM);
	ASSERT_NE(addr, nullptr);

	int length = 0;
	do {
		NetAddressIn *addr_in = NET_address_in_new_ex("0.0.0.0", 0, NET_address_family(addr));
		if (!addr_in) {
			continue;
		}

		NetSocket *sock = NET_socket_new(addr);
		if (sock) {
			do {
				if (NET_socket_bind(sock, addr_in) != 0) {
					break;
				}
				if (NET_socket_listen(sock) != 0) {
					break;
				}
				length++;
			} while (false);

			NET_socket_free(sock);
		}

		NET_address_in_free(addr_in);
	} while (NET_address_next(addr));

	EXPECT_GT(length, 0);  // At least one socket should listen.

	NET_address_free(addr);
	NET_exit();
}

}
