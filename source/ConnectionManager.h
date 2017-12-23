#pragma once

#include "interface/Tubes.h" // TODODB: Remove this when callbacks have been changed to no longer depend upon the externals in MUTility (Causes struct redefinition is TubesTypes.h is included instead)

#include "InternalTubesTypes.h"
#include "Connection.h"
#include <MUtilityExternal/CallbackRegister.h>
#include <atomic>
#include <mutex>

class TubesMessageReplicator;

class ConnectionManager
{
public:
	~ConnectionManager();

	void		VerifyNewConnections( bool isHost, TubesMessageReplicator& replicator );

	void		RequestConnection( const std::string& address, Port port );
	void		StartListener( Port port );
	void		StopAllListeners(); // TODODB: Add option to stop only specific listener
	
	ConnectionCallbackHandle RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction );
	bool UnregisterConnectionCallback( ConnectionCallbackHandle handle );

	Connection* GetConnection( ConnectionID connectionID ) const;
	const std::unordered_map<ConnectionID, Connection*>& GetVerifiedConnections() const;

private:

	void Connect( const std::string& address, Port port );
	void Listen( Socket listeningsSocket, std::atomic_bool* shouldTerminate );

	void ShutdownAndCloseSocket( Socket socket );

	std::vector<std::pair<Connection*, ConnectionState>> m_UnverifiedConnections;
	std::mutex m_UnverifiedConnectionsLock;

	std::unordered_map<ConnectionID, Connection*> m_Connections;

	std::unordered_map<Port, Listener*> m_ListenerMap;

	CallbackRegister<ConnectionCallbackTag, void, uint32_t> m_ConnectionCallbacks;

	ConnectionID m_NextConnectionID = 1;
}; 