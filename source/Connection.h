#pragma once
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

	bool					Connect();
	void					Disconnect();
	bool					SetBlockingMode( bool shouldBlock );
	bool					SetNoDelay(); // TODODB: Make this function take a parameter so it can be used to turn nodelay off


	SendResult				SerializeAndSendMessage(const Message& message, MessageReplicator& replicator);
	ReceiveResult			Receive(const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators, Message*& outMessage);

	SendResult				SendQueuedMessages();

	bool					operator == ( const Connection& other ) const { return this->m_Address == other.m_Address && this->m_Socket == other.m_Socket; }
	bool					operator != ( const Connection& other ) const { return this->m_Address != other.m_Address || this->m_Socket != other.m_Socket; }

	Address					GetAddress() const { return m_Address; }
	Port					GetPort() const { return m_Port; }

	static uint32_t			ConnectionTimeout;
private:
	
	SendResult				SendSerializedMessage(MUtility::Byte* serializedMessage, MessageSize messageSize);

	Socket					m_Socket;
	Address					m_Address;
	Port					m_Port;
	struct sockaddr_in		m_Sockaddr;
	ReceiveBuffer			m_ReceiveBuffer;

	std::queue<std::pair<MUtility::Byte*, MessageSize>> unsentMessages; // TODODB: Create struct holding Byte* and Messagesize to get rid of code like".front().first"
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