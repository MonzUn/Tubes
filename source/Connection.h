#pragma once
#include "InternalTubesTypes.h"
#include <queue>
#if PLATFORM == PLATFORM_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h> // for sockaddr_in
#endif

struct Connection
{
	Connection( Socket connectionSocket, const std::string& destinationAddress, Port destinationPort );
	Connection( Socket connectionSocket, const sockaddr_in& destination );
	~Connection();

	bool					SetBlockingMode( bool shouldBlock );
	bool					SetNoDelay(); // TODODB: Make this function take a parameter so it can be used to turn nodelay off

	bool					operator == ( const Connection& other ) const { return this->address == other.address && this->socket == other.socket; }
	bool					operator != ( const Connection& other ) const { return this->address != other.address || this->socket != other.socket; }

	Socket					socket;
	Address					address;
	Port					port;
	struct sockaddr_in		sockaddr;
	ReceiveBuffer			receiveBuffer;

	std::queue<std::pair<Byte*, MessageSize>> unsentMessages; // TODODB: Create struct holding Byte* and Messagesize to get rid of code like".front().first"
};