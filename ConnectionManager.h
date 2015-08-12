#pragma once
#include <mutex>
#include <atomic>
#include <memory/Alloc.h>
#include <utility/CallbackRegister.h>
#include "TubesTypes.h"
#include "InternalTubesTypes.h"
#include "Connection.h"

class TubesMessageReplicator;

class ConnectionManager {
public:
	~ConnectionManager();

	void		VerifyNewConnections( bool isHost, TubesMessageReplicator& replicator );

	void		RequestConnection( const tString& address, Port port );
	void		StartListener( Port port );
	void		StopAllListeners(); // TODODB: Add option to stop only specific listener
	
	ConnectionCallbackHandle RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction );
	bool UnregisterConnectionCallback( ConnectionCallbackHandle handle );

	Connection* GetConnection( ConnectionID connectionID ) const;
	const pMap<ConnectionID, Connection*>& GetVerifiedConnections() const;

private:

	void Connect( const tString& address, Port port );
	void Listen( Socket listeningsSocket, std::atomic_bool* shouldTerminate );

	void ShutdownAndCloseSocket( Socket socket );

	pVector<std::pair<Connection*, ConnectionState>> m_UnverifiedConnections;
	std::mutex m_UnverifiedConnectionsLock;

	pMap<ConnectionID, Connection*> m_Connections;

	rMap<Port, Listener*> m_ListenerMap;

	CallbackRegister<ConnectionCallbackTag, void, uint32_t> m_ConnectionCallbacks;

	ConnectionID m_NextConnectionID = 1;
}; 