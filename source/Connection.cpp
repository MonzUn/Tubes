#include "Connection.h"
#include "TubesUtility.h"
#include "TubesErrors.h"
#include <MUtilityLog.h>

#if PLATFORM != PLATFORM_WINDOWS
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#endif

#define TUBES_LOG_CATEGORY_CONNECTION "TubesConnection"

using namespace TubesUtility;
Connection::Connection( Socket connectionSocket, const std::string& destinationAddress, Port destinationPort )
{
	socket	= connectionSocket;
	address	= TubesUtility::IPv4StringToAddress( destinationAddress ); // TODODB: Handle ipv6
	port	=  destinationPort;

	memset( &sockaddr, 0, sizeof( sockaddr ) );
	sockaddr.sin_family			= AF_INET;
	sockaddr.sin_addr.s_addr	= htonl( address );
	sockaddr.sin_port			= htons( port );
}

Connection::Connection( Socket connectionSocket, const sockaddr_in& destination )
{
	socket	= connectionSocket;
	address	= ntohl( destination.sin_addr.s_addr );
	port	= ntohs( destination.sin_port );		// Local port if destination is a received connection

	memset( &sockaddr, 0, sizeof( sockaddr_in ) );
	sockaddr.sin_family			= AF_INET;
	sockaddr.sin_addr.s_addr	= destination.sin_addr.s_addr;
	sockaddr.sin_port			= destination.sin_port;
}

Connection::~Connection()
{
	while(!unsentMessages.empty())
	{
		free( unsentMessages.front().first );
		unsentMessages.pop();
	}
}

bool Connection::SetBlockingMode( bool shouldBlock )
{
	int result;
#if PLATFORM == PLATFORM_WINDOWS
	unsigned long nonBlocking = static_cast<unsigned long>( !shouldBlock );
	result = ioctlsocket( socket, FIONBIO, &nonBlocking );
#else
	shouldBlock ? result = fcntl( socket, F_SETFL, fcntl( socket, F_GETFL, 1 ) | O_NONBLOCK ) : result = fcntl( socket, F_SETFL, fcntl( socket, F_GETFL, 0 ) | O_NONBLOCK );
#endif
	if ( result != 0 )
	{
		MLOG_ERROR( "Failed to set socket to non blocking mode", TUBES_LOG_CATEGORY_CONNECTION );
	}

	return result == 0;
}

bool Connection::SetNoDelay()
{
	bool returnValue = true;

	int flag	= 1;
	int result	= setsockopt( socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>( &flag ), sizeof( int ) );
	if ( result < 0 )
	{
		LogAPIErrorMessage( "Failed to set TCP_NODELAY for socket with destination " + AddressToIPv4String(address) + " (Error: " << result + ")", TUBES_LOG_CATEGORY_CONNECTION );
		returnValue = false;
	}
	return returnValue;
}