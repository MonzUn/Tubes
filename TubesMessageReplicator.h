#pragma once
#include <messaging/MessageReplicator.h>

#define REPLICATOR_DEBUG 1

class TubesMessageReplicator : public MessageReplicator {
public:
	TubesMessageReplicator() : MessageReplicator( TubesMessageReplicatorID ) {};

	Byte*		SerializeMessage( const Message* message, uint64_t* outMessageSize = nullptr, Byte* optionalWritingBuffer = nullptr ) override;
	Message*	DeserializeMessage( const Byte* const buffer ) override;
	uint64_t	CalculateMessageSize( const Message& message ) const override;

	const static ReplicatorID TubesMessageReplicatorID = 0;
};