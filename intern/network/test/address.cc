#include "NET_address.h"

#include "gtest/gtest.h"

namespace {

TEST(Address, Get) {
	NetAddress *addr = NET_address_new_ex("127.0.0.1", "3000", AF_UNSPEC, SOCK_STREAM);

	int length = 0;
	do {
		length++;
	} while(!NET_address_next(addr));

	EXPECT_GT(length, 0); // At least one address should be found!

	NET_address_free(addr);
}

}
