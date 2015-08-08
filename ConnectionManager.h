#pragma once
#include <memory/Alloc.h>
#include <mutex>
#include <atomic>
#include "TubesTypes.h"
#include "Connection.h"

class ConnectionManager {
public:
	void		VerifyNewConnections( bool isHost );

	void		RequestConnection( const tString& address, Port port );
	void		StartListener( Port port );
	void		StopAllListeners(); // TODODB: Add option to stop only specific listener

	Connection* GetConnection( ConnectionID connectionID ) const;

private:

	void Connect( const tString& address, Port port );
	void Listen( Socket listeningsSocket, std::atomic_bool* shouldTerminate );

	void ShutdownAndCloseSocket( Socket socket );

	pVector<std::pair<Connection*, ConnectionState>> m_UnverifiedConnections;
	std::mutex m_UnverifiedConnectionsLock;

	pMap<ConnectionID, Connection*> m_Connections;

	rMap<Port, Listener*> m_ListenerMap;
};