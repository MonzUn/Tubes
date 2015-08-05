#include "Communication.h"
#include <messaging/Message.h>
#include <messaging/MessageReplicator.h>
#include <memory/Alloc.h>
#include "Connection.h"
#include "TubesUtility.h"

#if PLATFORM == PLATFORM_WINDOWS
	#define SENDFLAGS				0
#elif PLATFORM == PLATFORM_LINUX
	#define SENDFLAGS				MSG_NOSIGNAL
#endif

using namespace TubesUtility;

void Communication::SendTubesMessage( Connection& connection, const Message& message, MessageReplicator& replicator ) {
	uint64_t messageSize;
	Byte* serializedMessage = replicator.SerializeMessage( &message, &messageSize );

	if ( serializedMessage == nullptr ) {
		LogWarningMessage( "Failed to send message of type" ); // TODODB: Print message type here when conversion from tString to rString bullshittery is resolved
		tFree( serializedMessage );
	}
	
	SendRawData( connection, serializedMessage, static_cast<int32_t>( messageSize ) );
}

void Communication::SendRawData( Connection& connection, const Byte* const data, int32_t dataSize ) {
	if ( connection.socket == INVALID_SOCKET ) {
		LogErrorMessage( "Attempted to send message through invalid socket. (Destination =  " + AddressToIPv4String( connection.address ) + " )" );
	}

	int32_t bytesSent = send( connection.socket, data, dataSize, SENDFLAGS );
	if ( bytesSent != dataSize ) {
		int error = GET_NETWORK_ERROR;
		if ( error == TUBES_ECONNECTIONABORTED || error == TUBES_EWOULDBLOCK || error == EPIPE || error == TUBES_ECONNRESET ) {
			// TODODB: Disconnect the socket

		} else {
			LogErrorMessage( "Sending of packet with length " + rToString( dataSize ) + " and destination " + AddressToIPv4String( connection.address ) + " failed" );
		}
	}
}