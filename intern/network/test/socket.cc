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
		NetAddressIn *addr_in = NET_address_in_new_ex("0.0.0.0", 6969, NET_address_family(addr));
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

TEST(Socket, Accept) {
	NET_init();

	auto server = NET_socket_new_ex(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT_NE(server, nullptr);

	auto addrin = NET_address_in_new_ex("0.0.0.0", 6969, AF_INET);
	ASSERT_NE(addrin, nullptr);
	ASSERT_EQ(NET_socket_bind(server, addrin), 0);
	ASSERT_EQ(NET_socket_listen(server), 0);

	auto client = NET_socket_new_ex(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT_NE(client, nullptr);

	auto addr = NET_address_new_ex("127.0.0.1", "6969", AF_INET, SOCK_STREAM);
	ASSERT_NE(addr, nullptr);

	/**
	 * Even though `connect` is a blocking function and we haven't accepted the connection yet,
	 * `NET_socket_listen` has already been called.
	 *
	 * Originally, this was tested using pthreads to spawn a separate client thread that we would join later.
	 * However, since this setup works reliably on both Windows and Unix systems,
	 * we reverted to a simple loop with non-blocking server-side polling.
	 * 
	 * 																								
	 * 															 .;X&&&&&&$+;.                      
	 * 														 .$&;             :+&$:                 
	 * 													   +&.                     X$.              
	 * 													 :&                          ;$             
	 * 													+x                             &            
	 * 												   +x                               &           
	 * 												  :$                                +;          
	 * 	 .$;;;;;;;;;;;;;;;;;;;;;;;;;;&:               x;                                 $          
	 * 	+&&&&$&&&&&&&&&&&&&&&&&&&&&&+;x               &                                  &          
	 * 	+.                        $;x:X              :&     :;;;..    .XX$;:x            $          
	 * 	;:                       X::$:+:              & .   x    :;         X           :+          
	 * 	;:                   ...X:::X;:X              $ .   X     $.:      x:           &.          
	 * 	.X                      X::::X:X              ;$$$X        :$$++&$;            :X           
	 * 	 x                      :X:::$:+;              $                              .&            
	 * 	 .X                      &:::x+:$              .&  .&$$$$&X.                 :&             
	 * 	  X    ;+;;;;;;;;;;;X+   ;::::$:X                &&:        XX              xX              
	 * 	  .+   x            $$    &:::X:;+               $&;          X+          ;&.               
	 * 	   &   X            $+;   $:::+x:&              .$ :&:         .&       $&:                 
	 * 		;  X            X:X    x:::$:X              +    .&X.        &  ;&$   $:                
	 * 		& :+           :+:X    &:::X::+             $       .+&&$$$&&x&+$      &:               
	 * 		:;+:           x;:X    +;::;x:&             &                 $$       X$               
	 * 		 $X            &:;+     X:::&:x:            &               .&.;+     :x:X              
	 * 		 ;$            $:X:     X;::x::x            &              x+   X:    +; X.             
	 * 		  x            X:$      .x::;X:&            X            :$     :$    $. :X             
	 * 		 ;;           :;:X::::;;+&;::&;X            ;;         .$;       X.  .x   $             
	 * 		 X:           X:;                           .&        &;         :$  &:   X             
	 * 		 $            &:X&&&&&&&&&&&&&&&&&&&+.       &.    ;&:            $ ;;    +:            
	 * ======$xxxxxxxxxxxx&+:xX===================;xX====;X==$$===============X+$=====++=========   
	 * 	   $.              x:::+&x++++++++++++++++xx;     ;+                  :;         :+$&X;:$.  
	 * 	 +&+++++++++++++xxx&$$+:                                                 ;XXX+;:::::::::$.  
	 *
	 *
	 * \remarks #NET_socket_connect blocks only until the server is ready to accept.
	 */
	EXPECT_EQ(NET_socket_connect((NetSocket *)client, addr), 0);

	while (!NET_socket_select(server, 0)) {
	}
	auto peer = NET_socket_accept(server);
	ASSERT_NE(peer, nullptr);
	NET_socket_free(peer);

	NET_socket_free(client);
	NET_address_free(addr);
	NET_address_in_free(addrin);
	NET_socket_free(server);
	NET_exit();
}

}
