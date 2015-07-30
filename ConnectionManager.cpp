#include "ConnectionManager.h"
#include "TubesUtility.h"
#include "Connection.h"

#if PLATFORM == PLATFORM_WINDOWS
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == WSAEWOULDBLOCK )
#elif PLATFORM == PLATFORM_LINUX
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == EINPROGRESS )
#endif

#define CONNECTION_TIMEOUT_SECONDS 2 // TODODB: Make this a settable variable

using namespace TubesUtility;
Connection* ConnectionManager::Connect( const tString& address, Port port ) {
	Connection* connection = pNew( Connection, address, port );

	// Set up the socket
	connection->socket = static_cast<int64_t>( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ); // Adress Family = INET and the protocol to be used is TCP
	if ( connection->socket <= 0 ) {
		LogErrorMessage( "Failed to create socket" );
		pDelete( connection );
		return nullptr;
	}

	connection->SetBlockingMode( false );

	// Set up timeout variables
	timeval timeOut;
	timeOut.tv_sec	= CONNECTION_TIMEOUT_SECONDS;
	timeOut.tv_usec = 0;

	fd_set set;
	FD_ZERO( &set );
	FD_SET( connection->socket, &set );

	// Attempt to connect
	LogInfoMessage( "Attempting to connect to " + address );

	int result;
	if ( ( result = connect( connection->socket, reinterpret_cast<sockaddr*>( &connection->sockaddr ), sizeof( sockaddr_in ) ) ) == INVALID_SOCKET ) {
		if ( !SHOULD_WAIT_FOR_TIMEOUT ) {
			pDelete( connection );
			return nullptr;
		}
	}
	result = select( static_cast<int>( connection-> socket + 1 ), NULL, &set, NULL, &timeOut );

	// Check result of the connection
	if ( result == 0 ) {
		LogInfoMessage( "Connection attempt to " + address + " timed out" );
		pDelete( connection );
		return nullptr;
	} else if ( result < 0 ) {
		LogErrorMessage( "Connection attempt to " + address + " failed" );
		pDelete( connection );
		return nullptr;
	}

	connection->SetNoDelay();

	LogInfoMessage( "Connection attempt to " + address + " was successfull!" );
	return connection;
}