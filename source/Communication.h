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
	SendResult		SendTubesMessage( Connection& connection, const Message& message, MessageReplicator& replicator ); // "Tubes" added to name in order to avoid conflict with windows define "SendMessage"
	ReceiveResult	Receive( Connection& connection, const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators, Message*& outMessage );
}

enum class SendResult
{
	Sent,
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