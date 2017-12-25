#include "Communication.h"
#include "interface/messaging/Message.h"
#include "interface/messaging/MessageReplicator.h"
#include "Connection.h"
#include "TubesUtility.h"
#include <MUtilitySerialization.h>

#if PLATFORM == PLATFORM_WINDOWS
	#define SEND_FLAGS				0
	#define RECEIVE_FLAGS			0
#elif PLATFORM == PLATFORM_LINUX
	#define SEND_FLAGS				MSG_NOSIGNAL
	#define	RECEIVE_FLAGS 0
#endif

using namespace TubesUtility;

void Communication::SendTubesMessage( Connection& connection, const Message& message, MessageReplicator& replicator )
{
	int32_t messageSize;
	Byte* serializedMessage = replicator.SerializeMessage( &message, &messageSize );

	if ( serializedMessage == nullptr ) {
		LogWarningMessage( "Failed to send message of type" ); // TODODB: Print message type here when conversion from tString to rString bullshittery is resolved
		free( serializedMessage );
		return;
	}
	
	SendRawData( connection, serializedMessage, messageSize );
	free(serializedMessage);
}

void Communication::SendRawData( Connection& connection, const Byte* const data, int32_t dataSize )
{
	if ( connection.socket == INVALID_SOCKET )
		LogErrorMessage( "Attempted to send message through invalid socket. (Destination =  " + AddressToIPv4String( connection.address ) + " )" );

	int32_t bytesSent = send( connection.socket, reinterpret_cast<const char*>(data), dataSize, SEND_FLAGS );
	if ( bytesSent != dataSize )
	{
		int error = GET_NETWORK_ERROR;
		if ( error == TUBES_ECONNECTIONABORTED || error == TUBES_EWOULDBLOCK || error == EPIPE || error == TUBES_ECONNRESET )
		{
			// TODODB: Disconnect the socket
		}
		else
			LogErrorMessage( "Sending of packet with length " + rToString( dataSize ) + " and destination " + AddressToIPv4String( connection.address ) + " failed" );
	}
}

Message* Communication::Receive( Connection& connection, const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators )
{
	if ( connection.socket == INVALID_SOCKET )
	{
		LogErrorMessage( "Attempted to receive from invalid socket" );
		return nullptr;
	}

	int32_t byteCountReceived;
	if ( connection.receiveBuffer.ExpectedHeaderBytes > 0 ) // If we are waiting for header data
	{ 
		byteCountReceived = recv( connection.socket, reinterpret_cast<char*>( connection.receiveBuffer.Walker ), connection.receiveBuffer.ExpectedHeaderBytes, RECEIVE_FLAGS ); // Attempt to receive header

		if ( byteCountReceived == -1 ) // No data was ready to be received or there was an error
		{ 
			int error = GET_NETWORK_ERROR;
			if ( error != TUBES_EWOULDBLOCK )
			{
				if ( error == TUBES_ECONNECTIONABORTED || error == TUBES_ECONNRESET ) // TODODB: Why isn't ECONNRESET checked when receiving payload data?
				{
					// TODODB: Disconnect the socket
					LogInfoMessage( "Connection to " + TubesUtility::AddressToIPv4String( connection.address ) + " was aborted" )
				}
				else
					LogErrorMessage( "An unhandled error occured while receiving header data" );
			}
			return nullptr;
		}

		if ( byteCountReceived == connection.receiveBuffer.ExpectedHeaderBytes ) // We received the full header
		{ 
			// Get the size of the packet (Embedded as first part) and create a buffer of that size
			connection.receiveBuffer.PayloadData = static_cast<Byte*>( malloc( connection.receiveBuffer.ExpectedPayloadBytes ) );

			connection.receiveBuffer.Walker = connection.receiveBuffer.PayloadData; // Walker now points to the new buffer since that is where we will want to write on the next recv

			// Write down the size at the beginning so the serialization is done properly
			MUtilitySerialization::CopyAndIncrementDestination( connection.receiveBuffer.Walker, &connection.receiveBuffer.ExpectedPayloadBytes, sizeof( MessageSize ) );
			connection.receiveBuffer.ExpectedPayloadBytes -= sizeof( MessageSize ); // We have already received the size variable

			connection.receiveBuffer.ExpectedHeaderBytes = 0; // Reset the expected header bytes variable so it indicates that payload data is being received now
		}
		else // Only a part of the header was received. Account for this and handle it in an upcoming call of this function
		{ 
			connection.receiveBuffer.ExpectedHeaderBytes	-= byteCountReceived;
			connection.receiveBuffer.Walker					+= byteCountReceived;
			return nullptr;
		}
	}

	byteCountReceived = recv( connection.socket, reinterpret_cast<char*>( connection.receiveBuffer.Walker ), connection.receiveBuffer.ExpectedPayloadBytes, RECEIVE_FLAGS ); // Attempt to receive payload

	if ( byteCountReceived == -1 ) // No data was ready to be received or there was an error // TODODB: This code is almost duplicated. See if it can be removed
	{ 
		int error = GET_NETWORK_ERROR;
		if ( error != TUBES_EWOULDBLOCK )
		{
			if ( error == TUBES_ECONNECTIONABORTED )
			{
				// TODODB: Disconnect the socket
				LogInfoMessage( "Connection to " + TubesUtility::AddressToIPv4String( connection.address ) + " was aborted" )
			}
			else
				LogErrorMessage( "An unhandled error occured while receiving payload data" );
		}
		return nullptr;
	}

	// If all data was received. Clean up, prepare for next call and return the buffer as a packet (Will need to be cast to the correct type on the outside using the Type field)
	if ( byteCountReceived == connection.receiveBuffer.ExpectedPayloadBytes )
	{
		// Read the replicator id
		ReplicatorID replicatorID;
		memcpy( &replicatorID, connection.receiveBuffer.PayloadData + sizeof( MessageSize ), sizeof( ReplicatorID ) ); // sizeof( MessageSize ) is for skipping the size variable embedded at the beginning of the buffer

		if ( replicators.find( replicatorID ) == replicators.end() ) // The requested replicator doesn't exist
		{ 
			LogErrorMessage( "Attempted to use replicator with id " + rToString( replicatorID ) + " but no such replicator exists" );
			free( connection.receiveBuffer.PayloadData );
			return nullptr;
		}

		Message* message = replicators.at( replicatorID )->DeserializeMessage( connection.receiveBuffer.PayloadData );
		free(connection.receiveBuffer.PayloadData);
		connection.receiveBuffer.ExpectedHeaderBytes	= sizeof( MessageSize ); // TODODB: Use default defines for these values
		connection.receiveBuffer.ExpectedPayloadBytes	= NOT_EXPECTING_PAYLOAD;
		connection.receiveBuffer.PayloadData			= nullptr;
		connection.receiveBuffer.Walker					= reinterpret_cast<Byte*>( &connection.receiveBuffer.ExpectedPayloadBytes );

		return message;
	}
	else // Only part of the payload was received. Account for this and attempt to receive the rest in an upcoming call of this function
	{ 
		connection.receiveBuffer.ExpectedPayloadBytes	-= byteCountReceived;
		connection.receiveBuffer.Walker					+= byteCountReceived;
		return nullptr;
	}
}