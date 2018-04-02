#pragma once
#include "Interface/Tubes.h" // TODODB: Remove this when callbacks have been changed to no longer depend upon the externals in MUTility (Causes struct redefinition is TubesTypes.h is included instead)
#include "InternalTubesTypes.h"
#include "Connection.h"
#include "Listener.h"
#include <MUtilityExternal/CallbackRegister.h>
#include <MUtilityLocklessQueue.h>

class TubesMessageReplicator;

class ConnectionManager // TODODB: Make this a namespace instead
{
public:
	ConnectionManager();
	~ConnectionManager();

	void VerifyNewConnections(TubesMessageReplicator& replicator);
	void HandleFailedConnectionAttempts();

	void RequestConnection(const std::string& address, Port port);
	void Disconnect(Tubes::DisconnectionType type, Tubes::ConnectionID connectionID);
	void DisconnectAll();

	bool StartListener(Port port);
	bool StopListener(Port port);
	bool StopAllListeners();

	Tubes::ConnectionCallbackHandle RegisterConnectionCallback(Tubes::ConnectionCallbackFunction callbackFunction);
	bool UnregisterConnectionCallback(Tubes::ConnectionCallbackHandle handle);
	Tubes::DisconnectionCallbackHandle RegisterDisconnectionCallback(Tubes::DisconnectionCallbackFunction callbackFunction);
	bool UnregisterDisconnectionCallback(Tubes::DisconnectionCallbackHandle handle);

	Connection* GetConnection(Tubes::ConnectionID ID) const;
	const std::unordered_map<Tubes::ConnectionID, Connection*>& GetVerifiedConnections() const;

	uint32_t GetVerifiedConnctionCount() const;
	std::string GetAddressOfConnection(Tubes::ConnectionID ID) const;
	uint16_t GetPortOfConnection(Tubes::ConnectionID ID) const;

	bool IsConnectionIDValid(Tubes::ConnectionID ID) const;

private:
	struct ConnectionAttemptData
	{
		ConnectionAttemptData() {}
		ConnectionAttemptData(const std::string& address, Port port) : Address(address), Port(port) {}
	
		ConnectionAttemptData& operator=(const ConnectionAttemptData& other)
		{
			Address = other.Address;
			Port	= other.Port;
			return *this;
		}

		std::string Address;
		Port Port = TUBES_INVALID_PORT;
	};

	void ProcessConnectionRequests();
	void Connect(const std::string& address, Port port);

	std::vector<std::pair<Connection*, ConnectionState>> m_UnverifiedConnections;
	std::unordered_map<Tubes::ConnectionID, Connection*> m_Connections;
	std::unordered_map<Port, Listener*> m_ListenerMap;

	CallbackRegister<Tubes::ConnectionCallbackTag, void, const Tubes::ConnectionAttemptResultData&> m_ConnectionCallbacks;
	CallbackRegister<Tubes::DisconnectionCallbackTag, void, const Tubes::DisconnectionData&> m_DisconnectionCallbacks;
	MUtility::LocklessQueue<Tubes::ConnectionAttemptResultData> FailedConnectionAttemptsQueue;

	Tubes::ConnectionID m_NextConnectionID = 1;

	std::thread						m_ConnectionThread;
	std::atomic<bool>				m_RunConnectionThread;
	std::unique_lock<std::mutex>	m_ConnectionAttemptLock;
	std::mutex						m_ConnectionAttemptLockMutex;
	std::condition_variable			m_ConnectionAttemptLockCondition;
	MUtility::LocklessQueue<ConnectionAttemptData> m_RequestedConnections;
};