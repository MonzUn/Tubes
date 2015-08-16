#include "TubesMessageReplicator.h"
#include "TubesUtility.h"
#include "TubesMessages.h"

using namespace DataSizes;
using namespace SerializationUtility;
using namespace Messages;

Byte* TubesMessageReplicator::SerializeMessage( const Message* message, MessageSize* outMessageSize, Byte* optionalWritingBuffer ) {
	// Attemt to get the message size
	MessageSize messageSize = CalculateMessageSize( *message );
	if ( messageSize == 0 ) {
		return nullptr;
	}

	// Set the out variable if applicable
	if ( outMessageSize != nullptr ) {
		*outMessageSize = messageSize;
	}

	// Create a buffer to hold the serialized data
	Byte* serializedMessage = optionalWritingBuffer == nullptr ? tAlloc( Byte, messageSize ) : optionalWritingBuffer;
	m_WritingWalker = serializedMessage;

	// Write the message size
	CopyAndIncrementDestination( m_WritingWalker, &messageSize, sizeof( MessageSize ) );

	// Write the replicator ID
	SerializationUtility::CopyAndIncrementDestination( m_WritingWalker, &message->Replicator_ID, sizeof( ReplicatorID ) );

	// Write the message type variable
	SerializationUtility::CopyAndIncrementDestination( m_WritingWalker, &message->Type, sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) );

	// Perform serialization specific to each message type (Use same order as in the type enums here)
	switch ( message->Type ) {
		case CONNECTION_ID: {
			const ConnectionIDMessage* idMessage = static_cast<const ConnectionIDMessage*>( message );
			CopyAndIncrementDestination( m_WritingWalker, &idMessage->ID, sizeof( ConnectionID ) );
		} break;

		default: {
			LogErrorMessage( "Failed to find serialization logic for message of type " + rToString( message->Type ) );
			if ( optionalWritingBuffer == nullptr ) { // Only free the memory buffer if it wasn't supplied as a parameter
				tFree( serializedMessage );
			}
			serializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	uint64_t differance = m_WritingWalker - serializedMessage;
	if ( differance != messageSize ) {
		LogErrorMessage( "SerializeMessage didn't write the expected amount of bytes" );
		assert( false );
	}
#endif

	m_WritingWalker = nullptr;
	return serializedMessage;
}

Message* TubesMessageReplicator::DeserializeMessage( const Byte* const buffer ) {
	m_ReadingWalker = buffer;

	// Read the message size
	MessageSize messageSize;
	CopyAndIncrementSource( &messageSize, m_ReadingWalker, sizeof( MessageSize ) );

	// Allocate a writing buffer that will become a message
	Message* deserializedMessage = reinterpret_cast<Message*>( tAlloc( Byte*, messageSize ) );

	// Read the replicator ID
	CopyAndIncrementSource( &deserializedMessage->Replicator_ID, m_ReadingWalker, sizeof( ReplicatorID ) );

	// Read the message type
	CopyAndIncrementSource( &deserializedMessage->Type, m_ReadingWalker, sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) );

	switch ( deserializedMessage->Type ) {
		case CONNECTION_ID: {
			ConnectionIDMessage* idMessage = static_cast<ConnectionIDMessage*>( deserializedMessage );
			CopyAndIncrementSource( &idMessage->ID, m_ReadingWalker, sizeof( ConnectionID ) );
		} break;

		default: {
			LogErrorMessage("Failed to find deserialization logic for message of type " + rToString( deserializedMessage->Type) )
			deserializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	uint64_t differance = m_ReadingWalker - buffer;
	if ( differance != messageSize ) {
		LogErrorMessage( "DeserializeMessage didn't read the expected amount of bytes");
		assert( false );
	}
#endif

	m_ReadingWalker = nullptr;
	return deserializedMessage;
}

int32_t TubesMessageReplicator::CalculateMessageSize( const Message& message ) const {
	int32_t messageSize = 0;

	// Add the size of the variables size, type and replicator ID
	messageSize += sizeof( MessageSize );
	messageSize += sizeof( ReplicatorID );
	messageSize += sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE );

	// Add size specific to message
	switch ( message.Type ) {
		case CONNECTION_ID : {
			messageSize += sizeof( ConnectionID );
		} break;

		default: {
			LogErrorMessage( "Failed to find size calculation logic for message of type " + rToString( message.Type ) );
			messageSize = 0;
		} break;
	}
	return messageSize;
}