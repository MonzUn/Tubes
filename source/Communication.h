#pragma once
#include "interface/messaging/MessagingTypes.h"
#include <MUtilityByte.h>
#include <stdint.h>
#include <unordered_map>

struct		Message;
struct		Connection;
class		MessageReplicator;
enum class	SendResult;
enum class	ReceiveResult;


namespace Communication
{
	SendResult		SerializeAndSendMessage( Connection& connection, const Message& message, MessageReplicator& replicator );
	SendResult		SendSerializedMessage( Connection& connection, Byte* serializedMessage, MessageSize messageSize );
	ReceiveResult	Receive( Connection& connection, const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators, Message*& outMessage );
}

enum class SendResult
{
	Sent,
	Queued,
	Disconnect,
	Error,
};

enum class ReceiveResult
{
	Fullmessage,
	PartialMessage,
	Empty,
	ForcefulDisconnect,
	GracefulDisconnect,
	Error,
};