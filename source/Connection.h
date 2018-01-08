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

	bool					operator == ( const Connection& other ) const { return this->m_address == other.m_address && this->m_socket == other.m_socket; }
	bool					operator != ( const Connection& other ) const { return this->m_address != other.m_address || this->m_socket != other.m_socket; }

	Address					GetAddress() const { return m_address; }
	Port					GetPort() const { return m_port; }

private:
	
	SendResult				SendSerializedMessage(Byte* serializedMessage, MessageSize messageSize);

	Socket					m_socket;
	Address					m_address;
	Port					m_port;
	struct sockaddr_in		m_sockaddr;
	ReceiveBuffer			m_receiveBuffer;

	std::queue<std::pair<Byte*, MessageSize>> unsentMessages; // TODODB: Create struct holding Byte* and Messagesize to get rid of code like".front().first"
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