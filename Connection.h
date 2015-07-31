#pragma once
#include <memory/Alloc.h>
#include "TubesTypes.h"
#if PLATFORM == PLATFORM_WINDOWS
#include <utility/RetardedWindowsIncludes.h>
#endif

struct Connection {
	Connection( Socket connectionSocket, const tString& destinationAddress, Port destinationPort );

	bool SetBlockingMode( bool shouldBlock );
	bool SetNoDelay(); // TODODB: Make this function take a parameter so it can be used to turn nodelay off

	bool			operator == ( const Connection& other ) const { return this->address == other.address && this->socket == other.socket; }
	bool			operator != ( const Connection& other ) const { return this->address != other.address || this->socket != other.socket; }

	Socket			socket;
	Address			address;
	Port			port;
	sockaddr_in		sockaddr;
};