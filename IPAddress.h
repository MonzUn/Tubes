#pragma once
#include <memory/Alloc.h>
#include "TubesTypes.h"
#if PLATFORM == PLATFORM_WINDOWS
#include <utility/RetardedWindowsIncludes.h>
#endif

struct IPAddress {
	Address			address;
	Port			port;
	sockaddr_in		sockaddr;

	bool			operator == ( const IPAddress& other ) const { return this->address == other.address && this->port == other.port; }
	bool			operator != ( const IPAddress& other ) const { return this->address != other.address || this->port != other.port; }
};