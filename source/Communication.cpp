#include "Communication.h"
#include "interface/messaging/Message.h"
#include "interface/messaging/MessageReplicator.h"
#include "Connection.h"
#include "TubesUtility.h"
#include <MUtilityLog.h>
#include <MUtilitySerialization.h>

#if PLATFORM == PLATFORM_WINDOWS
	#define SEND_FLAGS				0
	#define RECEIVE_FLAGS			0
#elif PLATFORM == PLATFORM_LINUX
	#define SEND_FLAGS				MSG_NOSIGNAL
	#define	RECEIVE_FLAGS 0
#endif

#define TUBES_LOG_CATEGORY_COMMUNICATION "TubesCommunication"

using namespace TubesUtility;

SendResult Communication::SerializeAndSendMessage( Connection& connection, const Message& message, MessageReplicator& replicator )
{
	if (connection.socket == INVALID_SOCKET)
	{
		MLOG_ERROR( "Attempted to send message through invalid socket. (Destination =  " + AddressToIPv4String(connection.address) + " )", TUBES_LOG_CATEGORY_COMMUNICATION );
		return SendResult::Error;
	}

	MessageSize messageSize;
	Byte* serializedMessage = replicator.SerializeMessage( &message, &messageSize );

	if ( serializedMessage == nullptr )
	{
		MLOG_WARNING("Failed to serialize message of type" << message.Type + ". The message will not be sent", TUBES_LOG_CATEGORY_COMMUNICATION);
		free( serializedMessage );
		return SendResult::Error;
	}

	if ( !connection.unsentMessages.empty() )
	{
		connection.unsentMessages.push( std::pair<Byte*, MessageSize>( serializedMessage, messageSize ) );
		return SendResult::Queued;
	}

	return SendSerializedMessage( connection, serializedMessage, messageSize );
}

SendResult Communication::SendSerializedMessage( Connection& connection, Byte* serializedMessage, MessageSize messageSize )
{
	if ( !connection.unsentMessages.empty() && connection.unsentMessages.front().first != serializedMessage )
		return SendResult::Queued; // Do not send messages out of order

	SendResult result;
	int32_t bytesSent = send( connection.socket, reinterpret_cast<const char*>( serializedMessage ), messageSize, SEND_FLAGS );
	if ( bytesSent == messageSize )
	{
		free( serializedMessage );
		result = SendResult::Sent;
	}
	else
	{
		int error = GET_NETWORK_ERROR;
		if ( error == TUBES_ECONNECTIONABORTED || error == EPIPE || error == TUBES_ECONNRESET )
		{
			result = SendResult::Disconnect;
		}
		else if ( error == TUBES_EWOULDBLOCK ) // IF EWOULDBLOCK is set, the send buffer is full
		{
			if( connection.unsentMessages.empty() ) // This is not a resend; put the message in the queue to be sent later
				connection.unsentMessages.push( std::pair<Byte*, MessageSize>( serializedMessage, messageSize ) );

			result = SendResult::Queued;
		}
		else
		{
			result = SendResult::Error;
			LogAPIErrorMessage( "Sending of packet with length " << messageSize << " and destination " << AddressToIPv4String( connection.address ) << " failed", TUBES_LOG_CATEGORY_COMMUNICATION );
		}
	}

	return result;
}

ReceiveResult Communication::Receive( Connection& connection, const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators, Message*& outMessage )
{
	if ( connection.socket == INVALID_SOCKET )
	{
		MLOG_ERROR( "Attempted to receive from invalid socket", TUBES_LOG_CATEGORY_COMMUNICATION );
		return ReceiveResult::Error;;
	}

	int32_t byteCountReceived;
	if ( connection.receiveBuffer.ExpectedHeaderBytes > 0 ) // If we are waiting for header data
	{ 
		byteCountReceived = recv( connection.socket, reinterpret_cast<char*>( connection.receiveBuffer.Walker ), connection.receiveBuffer.ExpectedHeaderBytes, RECEIVE_FLAGS ); // Attempt to receive header

		if (byteCountReceived == 0)
		{
			return ReceiveResult::GracefulDisconnect;
		}
		else if ( byteCountReceived == -1 ) // No data was ready to be received or there was an error
		{ 
			ReceiveResult result = ReceiveResult::Empty;
			int error = GET_NETWORK_ERROR;
			if ( error != TUBES_EWOULDBLOCK ) // If EWOULDBLOCK is set, the receive buffer is empty
			{
				if ( error == TUBES_ECONNECTIONABORTED || error == TUBES_ECONNRESET )
				{
					result = ReceiveResult::ForcefulDisconnect;
					MLOG_INFO( "A Connection with destination " + TubesUtility::AddressToIPv4String( connection.address ) + " was aborted", TUBES_LOG_CATEGORY_COMMUNICATION );
				}
				else
				{
					result = ReceiveResult::Error;
					LogAPIErrorMessage("An unhandled error occured while receiving header data", TUBES_LOG_CATEGORY_COMMUNICATION);
				}
			}
			return result;
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
			return ReceiveResult::PartialMessage;
		}
	}

	byteCountReceived = recv( connection.socket, reinterpret_cast<char*>( connection.receiveBuffer.Walker ), connection.receiveBuffer.ExpectedPayloadBytes, RECEIVE_FLAGS ); // Attempt to receive payload

	if (byteCountReceived == 0)
	{
		return ReceiveResult::GracefulDisconnect;
	}
	else if ( byteCountReceived == -1 ) // No data was ready to be received or there was an error // TODODB: This code is almost duplicated. See if it can be removed
	{
		ReceiveResult result = ReceiveResult::Empty;
		int error = GET_NETWORK_ERROR;
		if ( error != TUBES_EWOULDBLOCK ) // If EWOULDBLOCK is set, the receive buffer is empty
		{
			if ( error == TUBES_ECONNECTIONABORTED || error == TUBES_ECONNRESET)
			{
				result = ReceiveResult::ForcefulDisconnect;
				MLOG_INFO( "Connection to " + TubesUtility::AddressToIPv4String( connection.address ) + " was aborted", TUBES_LOG_CATEGORY_COMMUNICATION );
			}
			else
			{
				result = ReceiveResult::Error;
				LogAPIErrorMessage("An unhandled error occured while receiving payload data", TUBES_LOG_CATEGORY_COMMUNICATION);
			}
		}
		return result;
	}

	// If all data was received. Clean up, prepare for next call and return the buffer as a packet (Will need to be cast to the correct type on the outside using the Type field)
	if ( byteCountReceived == connection.receiveBuffer.ExpectedPayloadBytes )
	{
		// Read the replicator id
		ReplicatorID replicatorID;
		memcpy( &replicatorID, connection.receiveBuffer.PayloadData + sizeof( MessageSize ), sizeof( ReplicatorID ) ); // sizeof( MessageSize ) is for skipping the size variable embedded at the beginning of the buffer

		if ( replicators.find( replicatorID ) == replicators.end() ) // The requested replicator doesn't exist
		{ 
			MLOG_ERROR( "Attempted to use replicator with id " << replicatorID + " but no such replicator exists", TUBES_LOG_CATEGORY_COMMUNICATION );
			free( connection.receiveBuffer.PayloadData );
			return ReceiveResult::Error;
		}

		outMessage = replicators.at( replicatorID )->DeserializeMessage( connection.receiveBuffer.PayloadData );
		free( connection.receiveBuffer.PayloadData );
		connection.receiveBuffer.ExpectedHeaderBytes	= sizeof( MessageSize ); // TODODB: Use default defines for these values
		connection.receiveBuffer.ExpectedPayloadBytes	= NOT_EXPECTING_PAYLOAD;
		connection.receiveBuffer.PayloadData			= nullptr;
		connection.receiveBuffer.Walker					= reinterpret_cast<Byte*>( &connection.receiveBuffer.ExpectedPayloadBytes );

		return ReceiveResult::Fullmessage;
	}
	else // Only part of the payload was received. Account for this and attempt to receive the rest in an upcoming call of this function
	{ 
		connection.receiveBuffer.ExpectedPayloadBytes	-= byteCountReceived;
		connection.receiveBuffer.Walker					+= byteCountReceived;
		return ReceiveResult::PartialMessage;
	}
}