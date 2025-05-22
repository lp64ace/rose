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
		if (sock) {
			length++;
			NET_socket_free(sock);
		}
	} while (NET_address_next(addr));

	EXPECT_GT(length, 0);  // At least one socket should connect.

	NET_address_free(addr);
	NET_exit();
}

}
