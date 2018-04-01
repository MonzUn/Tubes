#pragma once
#include "Interface/TubesTypes.h"
#include "InternalTubesTypes.h"
#include "TubesMessageReplicator.h"
#include <queue>
#include <unordered_map>
#if PLATFORM == PLATFORM_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h> // for sockaddr_in
#endif

enum class	SendResult;
enum class	ReceiveResult;

class Connection
{
public:
	Connection( Socket connectionSocket, const std::string& destinationAddress, Port destinationPort );
	Connection( Socket connectionSocket, const sockaddr_in& destination );
	~Connection();

	Tubes::ConnectionAttemptResult	Connect();
	void							Disconnect();

	SendResult		SerializeAndSendMessage(const Message& message, MessageReplicator& replicator);
	ReceiveResult	Receive(const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators, Message*& outMessage);

	SendResult SendQueuedMessages();

	bool operator == ( const Connection& other ) const { return this->m_Address == other.m_Address && this->m_Socket == other.m_Socket; }
	bool operator != ( const Connection& other ) const { return this->m_Address != other.m_Address || this->m_Socket != other.m_Socket; }

	Address	GetAddress() const { return m_Address; }
	Port	GetPort() const { return m_Port; }

	bool	SetBlockingMode(bool shouldBlock);
	bool	SetNoDelay(bool noDelayOn);

	static uint32_t ConnectionTimeout;

private:
	struct MessageAndSize
	{
		MessageAndSize(MUtility::Byte* message, MessageSize messageSize) : Message(message), MessageSize(messageSize) {}

		MUtility::Byte* Message;
		MessageSize MessageSize;
	};

	SendResult SendSerializedMessage(MUtility::Byte* serializedMessage, MessageSize messageSize);

	Socket						m_Socket;
	Address						m_Address;
	Port						m_Port;
	struct sockaddr_in			m_Sockaddr;
	ReceiveBuffer				m_ReceiveBuffer;
	std::queue<MessageAndSize>	m_UnsentMessages;
};

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