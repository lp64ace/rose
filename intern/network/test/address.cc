#include "NET_socket.h"

#include "gtest/gtest.h"

namespace {

TEST(Address, Get) {
	NET_init();
	NetAddress *addr = NET_address_new_ex("127.0.0.1", "https", AF_UNSPEC, SOCK_STREAM);
	ASSERT_NE(addr, nullptr);

	int length = 0;
	do {
		length++;
	} while(NET_address_next(addr));

	EXPECT_GT(length, 0);  // At least one address should be found!

	NET_address_free(addr);
	NET_exit();
}

}
