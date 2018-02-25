#pragma once
#include "interface/messaging/MessageReplicator.h"

#define REPLICATOR_DEBUG 1

class TubesMessageReplicator : public MessageReplicator
{
public:
	TubesMessageReplicator() : MessageReplicator( TubesMessageReplicatorID ) {};

	MUtility::Byte*	SerializeMessage( const Message* message, MessageSize* outMessageSize = nullptr, MUtility::Byte* optionalWritingBuffer = nullptr ) override;
	Message*		DeserializeMessage( const MUtility::Byte* const buffer ) override;
	MessageSize		CalculateMessageSize( const Message& message ) const override;

	const static ReplicatorID TubesMessageReplicatorID = 0;
};