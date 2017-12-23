#pragma once
#include "interface/messaging/MessagingTypes.h"
#include <MUtilityByte.h>
#include <stdint.h>
#include <unordered_map>

struct	Message;
struct	Connection;
class   MessageReplicator;

namespace Communication
{
	void		SendTubesMessage( Connection& connection, const Message& message, MessageReplicator& replicator ); // "Tubes" added to name in order to avoid conflict with windows define "SendMessage"
	void		SendRawData( Connection& connection, const Byte* const data, int32_t dataSize );
	Message*	Receive( Connection& connection, const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators );
}