#pragma once
#include <stdint.h>
#include <utility/Byte.h>

struct	Message;
struct	Connection;
class   MessageReplicator;

namespace Communication {
	void		SendTubesMessage( Connection& connection, const Message& message, MessageReplicator& replicator ); // "Tubes" added to name in orde to avoid conflict with windows define "SendMessage"
	void		SendRawData( Connection& connection, const Byte* const data, int32_t dataSize );
	Message*	Receive( Connection& connection, MessageReplicator& replicator );
}