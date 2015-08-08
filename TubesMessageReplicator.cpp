#include "TubesMessageReplicator.h"
#include "TubesUtility.h"

using namespace DataSizes;
using namespace SerializationUtility;

Byte* TubesMessageReplicator::SerializeMessage( const Message* message, uint64_t* outMessageSize, Byte* optionalWritingBuffer ) {
	// Attemt to get the message size
	uint64_t messageSize = CalculateMessageSize( *message );
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
	WriteUint64( messageSize );

	// Write the message type variable
	SerializationUtility::CopyAndIncrementDestination( m_WritingWalker, &message->Type, sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) );

	// Write the replicator ID
	SerializationUtility::CopyAndIncrementDestination( m_WritingWalker, &message->Replicator_ID, sizeof( ReplicatorID ) );

	// Perform serialization specific to each message type (Use same order as in the type enums here)
	switch ( message->Type ) {

		// TODODB: Add message specific serialization here

		default: {
			LogErrorMessage( "Failed to find serialization logic for message of type " + rToString( message->Type ) );
			if ( optionalWritingBuffer == nullptr ) { // Only free the memory buffer if it wasn't supplied as a parameter
				tFree( serializedMessage );
			}
			serializedMessage = nullptr;
		} break;
	}


#if REPLICATOR_DEBUG
	size_t differance = m_WritingWalker - serializedMessage;
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
	uint64_t messageSize;
	ReadUint64( messageSize );

	// Allocate a writing buffer that will become a message
	Message* deserializedMessage = reinterpret_cast<Message*>( tAlloc( Byte*, messageSize ) );

	// Read the message type
	CopyAndIncrementSource( &deserializedMessage->Type, m_ReadingWalker, sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) );

	// Read the replicator ID variable
	CopyAndIncrementSource( &deserializedMessage->Replicator_ID, m_ReadingWalker, sizeof( ReplicatorID ) );

	switch ( deserializedMessage->Type ) {
		
		// TODODB: Add message specific deserialization here

		default: {
			LogErrorMessage("Failed to find deserialization logic for message of type " + rToString( deserializedMessage->Type) )
			deserializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	size_t differance = m_ReadingWalker - buffer;
	if ( differance != messageSize ) {
		LogErrorMessage( "DeserializeMessage didn't read the expected amount of bytes");
		assert( false );
	}
#endif

	m_ReadingWalker = nullptr;
	return deserializedMessage;
}

uint64_t TubesMessageReplicator::CalculateMessageSize( const Message& message ) const {
	uint64_t messageSize = 0;

	// Add the size of the variables size, type and replicator ID
	messageSize += INT_64_SIZE;
	messageSize += sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE );
	messageSize += sizeof( ReplicatorID );

	// Add size specific to message
	switch ( message.Type ) {
		
		// TODODB: Add message specific size measurements here

		default: {
			LogErrorMessage( "Failed to find size calculation logic for message of type " + rToString( message.Type ) );
			messageSize = 0;
		} break;
	}
	return messageSize;
}