#pragma once
#include "Interface/Tubes.h" // TODODB: Remove this when callbacks have been changed to no longer depend upon the externals in MUTility (Causes struct redefinition is TubesTypes.h is included instead)
#include "InternalTubesTypes.h"
#include "Connection.h"
#include "Listener.h"
#include <MUtilityExternal/CallbackRegister.h>
#include <MUtilityLocklessQueue.h>

using namespace Tubes;

class TubesMessageReplicator;

class ConnectionManager // TODODB: Make this a namespace instead
{
public:
	~ConnectionManager();

	void VerifyNewConnections( TubesMessageReplicator& replicator );
	void HandleFailedConnectionAttempts();
		 
	void RequestConnection( const std::string& address, Port port );
	void Disconnect( ConnectionID connectionID );
	void DisconnectAll();
		 
	bool StartListener(Port port);
	bool StopListener(Port port);
	bool StopAllListeners();
	
	ConnectionCallbackHandle RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction );
	bool UnregisterConnectionCallback( ConnectionCallbackHandle handle );
	DisconnectionCallbackHandle RegisterDisconnectionCallback( DisconnectionCallbackFunction callbackFunction);
	bool UnregisterDisconnectionCallback( DisconnectionCallbackHandle handle );
	ConnectionFailedCallbackHandle RegisterConnectionFailedCallback(ConnectionFailedCallbackFunction callbackFunction);
	bool UnregisterConnectionFailedCallback(ConnectionFailedCallbackHandle handle);

	Connection* GetConnection( ConnectionID connectionID ) const;
	const std::unordered_map<ConnectionID, Connection*>& GetVerifiedConnections() const;

	std::string GetAddressOfConnection(ConnectionID connectionID) const;
	uint16_t GetPortOfConnection(ConnectionID connectionID) const;

private:

	void Connect( const std::string& address, Port port );

	std::vector<std::pair<Connection*, ConnectionState>> m_UnverifiedConnections;
	std::unordered_map<ConnectionID, Connection*> m_Connections;
	std::unordered_map<Port, Listener*> m_ListenerMap;

	CallbackRegister<ConnectionCallbackTag, void, uint32_t> m_ConnectionCallbacks;
	CallbackRegister<DisconnectionCallbackTag, void, uint32_t> m_DisconnectionCallbacks;
	CallbackRegister<ConnectionFailedCallbackTag, void, const ConnectionAttemptResultData&> m_ConnectionFailedCallbacks;
	MUtility::LocklessQueue<ConnectionAttemptResultData> FailedConnectionAttemptsQueue;

	ConnectionID m_NextConnectionID = 1;
}; 