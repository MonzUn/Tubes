#pragma once
#include "InternalTubesTypes.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#define	MAX_LISTENING_BACKLOG 16 // How many incoming connections that can be connecting at the same time

class Connection;

class Listener
{
public:
	Listener();
	~Listener();

	bool StartListening(Port port);
	void StopListening();

	void FetchAcceptedConnections(std::vector<std::pair<Connection*, ConnectionState>>& outConnections);

private:
	void Listen();

	Socket				m_ListeningSocket;
	std::thread*		m_Thread;
	std::atomic<bool>*	m_ShouldTerminateListeningThread;

	std::mutex					m_AcceptedConnectionsLock;
	std::vector<Connection*>	m_AcceptedConnections;
};