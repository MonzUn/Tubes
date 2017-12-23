#include "TubesMessageReplicator.h"
#include "TubesTypes.h"
#include "TubesUtility.h"
#include "TubesMessages.h"
#include <MUtilitySerialization.h>
#include <cassert>

using namespace DataSizes;
using namespace MUtilitySerialization;
using namespace TubesMessages;

Byte* TubesMessageReplicator::SerializeMessage( const Message* message, MessageSize* outMessageSize, Byte* optionalWritingBuffer )
{
	// Attempt to get the message size
	MessageSize messageSize = CalculateMessageSize( *message );
	if ( messageSize == 0 )
		return nullptr;

	// Set the out variable if applicable
	if ( outMessageSize != nullptr )
		*outMessageSize = messageSize;

	// Create a buffer to hold the serialized data
	Byte* serializedMessage = (optionalWritingBuffer == nullptr) ? static_cast<Byte*>(malloc( messageSize ) ) : optionalWritingBuffer;
	m_WritingWalker = serializedMessage;

	// Write the message size
	CopyAndIncrementDestination( m_WritingWalker, &messageSize, sizeof( MessageSize ) );

	// Write the replicator ID
	CopyAndIncrementDestination(m_WritingWalker, &message->Replicator_ID, sizeof(ReplicatorID));

	// Write the message type variable
	MUtilitySerialization::CopyAndIncrementDestination( m_WritingWalker, &message->Type, sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) );

	// Perform serialization specific to each message type (Use same order as in the type enums here)
	switch ( message->Type )
	{
		case CONNECTION_ID:
		{
			const ConnectionIDMessage* idMessage = nullptr;
			idMessage = static_cast<const ConnectionIDMessage*>(message);
			CopyAndIncrementDestination(m_WritingWalker, &idMessage->ID, sizeof(ConnectionID));
		} break;

		default:
		{
			LogErrorMessage( "Failed to find serialization logic for message of type " + rToString( message->Type ) );
			if ( optionalWritingBuffer == nullptr ) // Only free the memory buffer if it wasn't supplied as a parameter
				free( serializedMessage );

			serializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	uint64_t differance = m_WritingWalker - serializedMessage;
	if ( differance != messageSize )
	{
		LogErrorMessage( "SerializeMessage didn't write the expected amount of bytes" );
		assert( false );
	}
#endif

	m_WritingWalker = nullptr;
	return serializedMessage;
}

Message* TubesMessageReplicator::DeserializeMessage( const Byte* const buffer )
{
	m_ReadingWalker = buffer;

	// Read the message size
	MessageSize messageSize;
	CopyAndIncrementSource( &messageSize, m_ReadingWalker, sizeof( MessageSize ) );

	// Read the replicator ID
	ReplicatorID replicatorID;
	CopyAndIncrementSource(&replicatorID, m_ReadingWalker, sizeof(ReplicatorID));

	Message* deserializedMessage = nullptr;

	// Read the message type
	uint64_t messageType;
	CopyAndIncrementSource( &messageType, m_ReadingWalker, sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) );
	switch ( messageType )
	{
		case CONNECTION_ID:
		{
			ConnectionID connectionID;
			CopyAndIncrementSource(&connectionID, m_ReadingWalker, sizeof(ConnectionID));
			deserializedMessage = new ConnectionIDMessage(connectionID);
		} break;

		default:
		{
			LogErrorMessage( "Failed to find deserialization logic for message of type " + rToString(deserializedMessage->Type) );
			deserializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	uint64_t differance = m_ReadingWalker - buffer;
	if ( differance != messageSize )
	{
		LogErrorMessage( "DeserializeMessage didn't read the expected amount of bytes");
		assert( false );
	}
#endif

	m_ReadingWalker = nullptr;

	return deserializedMessage;
}

int32_t TubesMessageReplicator::CalculateMessageSize( const Message& message ) const
{
	int32_t messageSize = 0;

	// Add the size of the variables size, type and replicator ID
	messageSize += sizeof(MessageSize);
	messageSize += sizeof(ReplicatorID);
	messageSize += sizeof(MESSAGE_TYPE_ENUM_UNDELYING_TYPE);

	// Add size specific to message
	switch ( message.Type )
	{
		case CONNECTION_ID :
		{
			messageSize += sizeof(ConnectionID);
		} break;

		default:
		{
			LogErrorMessage( "Failed to find size calculation logic for message of type " + rToString( message.Type ) );
			messageSize = 0;
		} break;
	}
	return messageSize;
}