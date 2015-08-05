#pragma once
#include <stdint.h>
#include <utility/Byte.h>

struct	Message;
struct	Connection;
class   MessageReplicator;

namespace Communication {
	void SendTubesMessage( Connection& connection, const Message& message, MessageReplicator& replicator );
	void SendRawData( Connection& connection, const Byte* const data, int32_t dataSize );
}