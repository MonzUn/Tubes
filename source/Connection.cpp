#include "Connection.h"
#include "TubesUtility.h"
#include "TubesErrors.h"
#include <MUtilityLog.h>
#include <MUtilitySerialization.h>

#if PLATFORM != PLATFORM_WINDOWS
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#else
#include <WinSock2.h>
#endif

#if PLATFORM == PLATFORM_WINDOWS
#define SEND_FLAGS				0
#define RECEIVE_FLAGS			0
#elif PLATFORM == PLATFORM_LINUX
#define SEND_FLAGS				MSG_NOSIGNAL
#define	RECEIVE_FLAGS 0
#endif

#define TUBES_LOG_CATEGORY_CONNECTION "TubesConnection"
#define CONNECTION_TIMEOUT_SECONDS 2 // TODODB: Make this a settable variable

#if PLATFORM == PLATFORM_WINDOWS
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == WSAEWOULDBLOCK )
#elif PLATFORM == PLATFORM_LINUX
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == EINPROGRESS )
#endif

using namespace TubesUtility;

// ---------- PUBLIC ----------

Connection::Connection( Socket connectionSocket, const std::string& destinationAddress, Port destinationPort )
{
	m_socket	= connectionSocket;
	m_address	= TubesUtility::IPv4StringToAddress( destinationAddress ); // TODODB: Handle ipv6
	m_port	=  destinationPort;

	memset( &m_sockaddr, 0, sizeof( sockaddr ) );
	m_sockaddr.sin_family		= AF_INET;
	m_sockaddr.sin_addr.s_addr	= htonl( m_address );
	m_sockaddr.sin_port			= htons( m_port );
}

Connection::Connection( Socket connectionSocket, const sockaddr_in& destination )
{
	m_socket	= connectionSocket;
	m_address	= ntohl( destination.sin_addr.s_addr );
	m_port		= ntohs( destination.sin_port );		// Local port if destination is a received connection

	memset( &m_sockaddr, 0, sizeof( sockaddr_in ) );
	m_sockaddr.sin_family		= AF_INET;
	m_sockaddr.sin_addr.s_addr	= destination.sin_addr.s_addr;
	m_sockaddr.sin_port			= destination.sin_port;
}

Connection::~Connection()
{
	while(!unsentMessages.empty())
	{
		free( unsentMessages.front().first );
		unsentMessages.pop();
	}
}

bool Connection::Connect() // TODODB: Doesn't this function block for the timeout period? Thread?
{
	// Set up timeout variables
	timeval timeOut;
	timeOut.tv_sec = CONNECTION_TIMEOUT_SECONDS;
	timeOut.tv_usec = 0;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(m_socket, &set);

	// Attempt to connect
	MLOG_INFO("Attempting to connect to " + AddressToIPv4String(m_address), TUBES_LOG_CATEGORY_CONNECTION);

	int result;
	if ((result = connect(m_socket, reinterpret_cast<sockaddr*>(&m_sockaddr), sizeof(sockaddr_in))) == INVALID_SOCKET)
	{
		if (!SHOULD_WAIT_FOR_TIMEOUT)
			return false;
	}
	result = select(static_cast<int>(m_socket + 1), NULL, &set, NULL, &timeOut);

	// Check result of the connection
	if (result == 0)
	{
		MLOG_INFO("Connection attempt to " + AddressToIPv4String(m_address) + " timed out", TUBES_LOG_CATEGORY_CONNECTION);
		return false;
	}
	else if (result < 0)
	{
		LogAPIErrorMessage("Connection attempt to " + AddressToIPv4String(m_address) + " failed", TUBES_LOG_CATEGORY_CONNECTION);
		return false;
	}

	return true;
}

void Connection::Disconnect()
{
	while (!unsentMessages.empty())
	{
		free(unsentMessages.front().first);
		unsentMessages.pop();
	}

	TubesUtility::ShutdownAndCloseSocket(m_socket);
}

bool Connection::SetBlockingMode( bool shouldBlock )
{
	int result;
#if PLATFORM == PLATFORM_WINDOWS
	unsigned long nonBlocking = static_cast<unsigned long>( !shouldBlock );
	result = ioctlsocket( m_socket, FIONBIO, &nonBlocking );
#else
	shouldBlock ? result = fcntl( socket, F_SETFL, fcntl( socket, F_GETFL, 1 ) | O_NONBLOCK ) : result = fcntl( socket, F_SETFL, fcntl( socket, F_GETFL, 0 ) | O_NONBLOCK );
#endif
	if ( result != 0 )
	{
		MLOG_ERROR( "Failed to set socket to non blocking mode", TUBES_LOG_CATEGORY_CONNECTION );
	}

	return result == 0;
}

bool Connection::SetNoDelay()
{
	bool returnValue = true;

	int flag	= 1;
	int result	= setsockopt( m_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>( &flag ), sizeof( int ) );
	if ( result < 0 )
	{
		LogAPIErrorMessage( "Failed to set TCP_NODELAY for socket with destination " + AddressToIPv4String(m_address) + " (Error: " << result + ")", TUBES_LOG_CATEGORY_CONNECTION );
		returnValue = false;
	}
	return returnValue;
}

// ---------- PUBLIC ----------

SendResult Connection::SerializeAndSendMessage(const Message& message, MessageReplicator& replicator)
{
	if (m_socket == INVALID_SOCKET)
	{
		MLOG_ERROR("Attempted to send message through invalid socket. (Destination =  " + AddressToIPv4String(m_address) + " )", TUBES_LOG_CATEGORY_CONNECTION);
		return SendResult::Error;
	}

	MessageSize messageSize;
	Byte* serializedMessage = replicator.SerializeMessage(&message, &messageSize);

	if (serializedMessage == nullptr)
	{
		MLOG_WARNING("Failed to serialize message of type" << message.Type + ". The message will not be sent", TUBES_LOG_CATEGORY_CONNECTION);
		free(serializedMessage);
		return SendResult::Error;
	}

	SendResult result = SendQueuedMessages();
	switch (result)
	{
		case SendResult::Sent: // No more unsent messages are left
		{
			result = SendSerializedMessage(serializedMessage, messageSize);
			if (result == SendResult::Queued)
				unsentMessages.push(std::pair<Byte*, MessageSize>(serializedMessage, messageSize));
		} break;

		case SendResult::Queued:
		{
			unsentMessages.push(std::pair<Byte*, MessageSize>(serializedMessage, messageSize));
			return SendResult::Queued;
		} break;


		case SendResult::Disconnect:
		case SendResult::Error:
		{
			free(serializedMessage);
		} break;

	default:
		break;
	}

	return result;
}

ReceiveResult Connection::Receive( const std::unordered_map<ReplicatorID, MessageReplicator*>& replicators, Message*& outMessage )
{
	if (m_socket == INVALID_SOCKET)
	{
		MLOG_ERROR("Attempted to receive from invalid socket", TUBES_LOG_CATEGORY_CONNECTION);
		return ReceiveResult::Error;
	}

	int32_t byteCountReceived;
	if (m_receiveBuffer.ExpectedHeaderBytes > 0) // If we are waiting for header data
	{
		byteCountReceived = recv(m_socket, reinterpret_cast<char*>(m_receiveBuffer.Walker), m_receiveBuffer.ExpectedHeaderBytes, RECEIVE_FLAGS); // Attempt to receive header

		if (byteCountReceived == 0)
		{
			MLOG_INFO("A Connection with destination " + TubesUtility::AddressToIPv4String(m_address) + " has disconnected gracefully", TUBES_LOG_CATEGORY_CONNECTION);
			return ReceiveResult::GracefulDisconnect;
		}
		else if (byteCountReceived == -1) // No data was ready to be received or there was an error
		{
			ReceiveResult result = ReceiveResult::Empty;
			int error = GET_NETWORK_ERROR;
			if (error != TUBES_EWOULDBLOCK) // If EWOULDBLOCK is set, the receive buffer is empty
			{
				if (error == TUBES_ECONNECTIONABORTED || error == TUBES_ECONNRESET)
				{
					result = ReceiveResult::ForcefulDisconnect;
					MLOG_INFO("A Connection with destination " + TubesUtility::AddressToIPv4String(m_address) + " has disconnected forcefully", TUBES_LOG_CATEGORY_CONNECTION);
				}
				else
				{
					result = ReceiveResult::Error;
					LogAPIErrorMessage("An unhandled error occured while receiving header data", TUBES_LOG_CATEGORY_CONNECTION);
				}
			}
			return result;
		}

		if (byteCountReceived == m_receiveBuffer.ExpectedHeaderBytes) // We received the full header
		{
			// Get the size of the packet (Embedded as first part) and create a buffer of that size
			m_receiveBuffer.PayloadData = static_cast<Byte*>(malloc(m_receiveBuffer.ExpectedPayloadBytes));

			m_receiveBuffer.Walker = m_receiveBuffer.PayloadData; // Walker now points to the new buffer since that is where we will want to write on the next recv

			// Write down the size at the beginning so the serialization is done properly
			MUtilitySerialization::CopyAndIncrementDestination(m_receiveBuffer.Walker, &m_receiveBuffer.ExpectedPayloadBytes, sizeof(MessageSize));
			m_receiveBuffer.ExpectedPayloadBytes -= sizeof(MessageSize); // We have already received the size variable

			m_receiveBuffer.ExpectedHeaderBytes = 0; // Reset the expected header bytes variable so it indicates that payload data is being received now
		}
		else // Only a part of the header was received. Account for this and handle it in an upcoming call of this function
		{
			m_receiveBuffer.ExpectedHeaderBytes -= byteCountReceived;
			m_receiveBuffer.Walker += byteCountReceived;
			return ReceiveResult::PartialMessage;
		}
	}

	byteCountReceived = recv(m_socket, reinterpret_cast<char*>(m_receiveBuffer.Walker), m_receiveBuffer.ExpectedPayloadBytes, RECEIVE_FLAGS); // Attempt to receive payload

	if (byteCountReceived == 0)
	{
		MLOG_INFO("A Connection with destination " + TubesUtility::AddressToIPv4String(m_address) + " has disconnected gracefully", TUBES_LOG_CATEGORY_CONNECTION);
		return ReceiveResult::GracefulDisconnect;
	}
	else if (byteCountReceived == -1) // No data was ready to be received or there was an error // TODODB: This code is almost duplicated. See if it can be removed
	{
		ReceiveResult result = ReceiveResult::Empty;
		int error = GET_NETWORK_ERROR;
		if (error != TUBES_EWOULDBLOCK) // If EWOULDBLOCK is set, the receive buffer is empty
		{
			if (error == TUBES_ECONNECTIONABORTED || error == TUBES_ECONNRESET)
			{
				result = ReceiveResult::ForcefulDisconnect;
				MLOG_INFO("Connection to " + TubesUtility::AddressToIPv4String(m_address) + " was aborted", TUBES_LOG_CATEGORY_CONNECTION);
			}
			else
			{
				result = ReceiveResult::Error;
				LogAPIErrorMessage("An unhandled error occured while receiving payload data", TUBES_LOG_CATEGORY_CONNECTION);
			}
		}
		return result;
	}

	// If all data was received. Clean up, prepare for next call and return the buffer as a packet (Will need to be cast to the correct type on the outside using the Type field)
	if (byteCountReceived == m_receiveBuffer.ExpectedPayloadBytes)
	{
		// Read the replicator id
		ReplicatorID replicatorID;
		memcpy(&replicatorID, m_receiveBuffer.PayloadData + sizeof(MessageSize), sizeof(ReplicatorID)); // sizeof( MessageSize ) is for skipping the size variable embedded at the beginning of the buffer

		if (replicators.find(replicatorID) == replicators.end()) // The requested replicator doesn't exist
		{
			MLOG_ERROR("Attempted to use replicator with id " << replicatorID + " but no such replicator exists", TUBES_LOG_CATEGORY_CONNECTION);
			free(m_receiveBuffer.PayloadData);
			return ReceiveResult::Error;
		}

		outMessage = replicators.at(replicatorID)->DeserializeMessage(m_receiveBuffer.PayloadData);
		free(m_receiveBuffer.PayloadData);
		m_receiveBuffer.ExpectedHeaderBytes = sizeof(MessageSize); // TODODB: Use default defines for these values
		m_receiveBuffer.ExpectedPayloadBytes = NOT_EXPECTING_PAYLOAD;
		m_receiveBuffer.PayloadData = nullptr;
		m_receiveBuffer.Walker = reinterpret_cast<Byte*>(&m_receiveBuffer.ExpectedPayloadBytes);

		return ReceiveResult::Fullmessage;
	}
	else // Only part of the payload was received. Account for this and attempt to receive the rest in an upcoming call of this function
	{
		m_receiveBuffer.ExpectedPayloadBytes -= byteCountReceived;
		m_receiveBuffer.Walker += byteCountReceived;
		return ReceiveResult::PartialMessage;
	}
}

SendResult Connection::SendQueuedMessages()
{
	SendResult sendResult = SendResult::Sent;
	bool breakLoop = false;
	while (!unsentMessages.empty() && !breakLoop)
	{
		sendResult = SendSerializedMessage(unsentMessages.front().first, unsentMessages.front().second);
		switch (sendResult)
		{
			case SendResult::Queued:
			case SendResult::Disconnect:
			case SendResult::Error:
			{
				breakLoop = true;
			} break;

			case SendResult::Sent:
			{
				unsentMessages.pop();
			} break;

			default:
				break;
		}
	}

	return sendResult;
}

// ---------- PRIVATE ----------

SendResult Connection::SendSerializedMessage(Byte* serializedMessage, MessageSize messageSize)
{
	SendResult result;
	int32_t bytesSent = send(m_socket, reinterpret_cast<const char*>(serializedMessage), messageSize, SEND_FLAGS);
	if (bytesSent == messageSize)
	{
		free(serializedMessage);
		result = SendResult::Sent;
	}
	else
	{
		int error = GET_NETWORK_ERROR;
		if (error == TUBES_ECONNECTIONABORTED || error == EPIPE || error == TUBES_ECONNRESET)
		{
			// TODODB: Do a recv() here so we know if the disconnect is gracefull or not
			result = SendResult::Disconnect;
		}
		else if (error == TUBES_EWOULDBLOCK) // IF EWOULDBLOCK is set, the send buffer is full
		{
			result = SendResult::Queued;
		}
		else
		{
			result = SendResult::Error;
			LogAPIErrorMessage("Sending of packet with length " << messageSize << " and destination " << AddressToIPv4String(m_address) << " failed", TUBES_LOG_CATEGORY_CONNECTION);
		}
	}

	return result;
}