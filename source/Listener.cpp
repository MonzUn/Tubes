#include "Listener.h"
#include "Connection.h"
#include "TubesUtility.h"
#include <MUtilityThreading.h>

#define LOG_CATEGORY_LISTENER "TubesListener"

Listener::Listener()
{
	m_Thread = nullptr;
	m_ListeningSocket = INVALID_SOCKET;
	m_ShouldTerminateListeningThread = new std::atomic<bool>(false);
}

Listener::~Listener()
{
	delete m_Thread;
	delete m_ShouldTerminateListeningThread;
}

bool Listener::StartListening(Port port)
{
	m_ListeningSocket = static_cast<Socket>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)); // Will be used to listen for incoming connections
	if (m_ListeningSocket == INVALID_SOCKET)
	{
		LogAPIErrorMessage("Failed to set up listening socket", LOG_CATEGORY_LISTENER);
		return false;
	}

	// Allow reuse of listening socket port
	char reuse = 1;
	setsockopt(m_ListeningSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(char)); // TODODB: Check return value

	// Set up the sockaddr for the listenign socket
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockAddr.sin_port = htons(port);

	// Bind the listening socket object to an actual socket.
	if (bind(m_ListeningSocket, (sockaddr*)&sockAddr, sizeof(sockAddr)) < 0)
	{
		LogAPIErrorMessage("Failed to bind listening socket", LOG_CATEGORY_LISTENER);
		return false;
	}

	// Start listening for incoming connections
	if (listen(m_ListeningSocket, MAX_LISTENING_BACKLOG) < 0)
	{
		LogAPIErrorMessage("Failed to start listening socket", LOG_CATEGORY_LISTENER);
		return false;
	}

	m_Thread = new std::thread(&Listener::Listen, this);

	MLOG_INFO("Listening for incoming connections on port " << port, LOG_CATEGORY_LISTENER);
	return true;
}

void Listener::StopListening()
{
	*m_ShouldTerminateListeningThread = true;
	TubesUtility::CloseSocket(m_ListeningSocket);
	MUtilityThreading::JoinThread(*m_Thread);
}

void Listener::FetchAcceptedConnections(std::vector<std::pair<Connection*, ConnectionState>>& outConnections)
{
	m_AcceptedConnectionsLock.lock();
	for (int i = 0; i < m_AcceptedConnections.size(); ++i)
	{
		outConnections.push_back(std::pair<Connection*, ConnectionState>(m_AcceptedConnections[i], ConnectionState::NewIncoming));
	}
	m_AcceptedConnections.clear();
	m_AcceptedConnectionsLock.unlock();
}

void Listener::Listen()
{
	do
	{
		Socket incomingConnectionSocket;
		sockaddr_in incomingConnectionInfo;
		socklen_t incomingConnectionInfoLength = sizeof(incomingConnectionInfo);

		// Wait for a connection or fetch one from the backlog
		incomingConnectionSocket = static_cast<Socket>(accept(m_ListeningSocket, reinterpret_cast<sockaddr*>(&incomingConnectionInfo), &incomingConnectionInfoLength)); // Blocking
		if (incomingConnectionSocket != INVALID_SOCKET)
		{
			Connection* connection = new Connection(incomingConnectionSocket, incomingConnectionInfo);
			connection->SetBlockingMode(false);
			connection->SetNoDelay(true);
			m_AcceptedConnectionsLock.lock();
			m_AcceptedConnections.push_back(connection);
			m_AcceptedConnectionsLock.unlock();
		}
		else
		{
			int error = GET_NETWORK_ERROR;
			if (error != TUBES_EINTR) // The socket was killed on purpose
				LogAPIErrorMessage("An incoming connection attempt failed", LOG_CATEGORY_LISTENER); // TODODB: See if we cant get the ip and print it here
		}
	} while (!*m_ShouldTerminateListeningThread);
}