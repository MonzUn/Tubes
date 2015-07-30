#pragma once
#include <memory/Alloc.h>
#include <mutex>
#include "TubesTypes.h"
#include "Connection.h"

enum ConnectionState {
	NEW_OUT,
	NEW_IN,
};

class ConnectionManager {
public:
	void RequestConnection( const tString& address, Port port );

private:
	void Connect( const tString& address, Port port );

	pVector<std::pair<Connection*, ConnectionState>> m_UnverifiedConnections;
	std::mutex m_UnverifiedConnectionsLock;
};