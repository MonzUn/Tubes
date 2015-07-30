#include "Connection.h"
#include "TubesUtility.h"
#include "TubesErrors.h"

using namespace TubesUtility;
Connection::Connection( const tString& destinationAddress, Port destinationPortport ) {
	// Convert adress and port and set them
	address	= TubesUtility::IPv4StringToAddress( destinationAddress ); // TODODB: Handle ipv6
	port	=  destinationPortport;

	memset( &sockaddr, 0, sizeof( sockaddr ) );
	sockaddr.sin_family			= AF_INET;
	sockaddr.sin_addr.s_addr	= htonl( address );
	sockaddr.sin_port			= htons( port );
}

bool Connection::SetBlockingMode( bool shouldBlock ) {
	int result;
#if PLATFORM == PLATFORM_WINDOWS
	unsigned long nonBlocking = static_cast<unsigned long>( !shouldBlock );
	result = ioctlsocket( socket, FIONBIO, &nonBlocking );
#else
	shouldBlock ? result = fcntl( m_Socket, F_SETFL, fcntl( m_Socket, F_GETFL, 1 ) | O_NONBLOCK ) : result = fcntl( m_Socket, F_SETFL, fcntl( m_Socket, F_GETFL, 0 ) | O_NONBLOCK );
#endif
	if ( result != 0 ) {
		LogErrorMessage( "Failed to set socket to non blocking mode" );
	}

	return result == 0;
}

bool Connection::SetNoDelay() {
	bool returnValue = true;

	int flag	= 1;
	int result	= setsockopt( socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>( &flag ), sizeof( int ) );
	if ( result < 0 ) {
		LogErrorMessage( "Failed to set TCP_NODELAY for socket with destination " + AddressToIPv4String( address ) + " (Error: " + rToString( result ) + ")" );
		returnValue = false;
	}
	return returnValue;
}