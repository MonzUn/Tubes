#pragma once
#include "Messaging/MessagingTypes.h"
#include "TubesTypes.h" // Exposes the relevant types to the external application
#include <string>

// TODODB: Add pinging and latency measurements
// TODODB: Standardize ordering of include statements
// TODODB: Standardize code (Remove whitespaces in paramter lists and whatnot)

class	MessageReplicator;
struct	Message;

namespace Tubes // TODOD: Remove redundant "connection" from connectionID parameters
{
	bool Initialize();
	void Shutdown();
	void Update();

	void SendToConnection(const Message* message, ConnectionID destinationConnectionID);
	void SendToAll(const Message* message, ConnectionID exception = TUBES_INVALID_CONNECTION_ID);
	void Receive(std::vector<Message*>& outMessages, std::vector<ConnectionID>* outSenderIDs = nullptr);

	void RequestConnection(const std::string& address, uint16_t port);
	bool StartListener(uint16_t port);
	bool StopListener(uint16_t port);
	bool StopAllListeners();
	void Disconnect(ConnectionID connectionID);
	void DisconnectAll();

	void RegisterReplicator(MessageReplicator* replicator);

	ConnectionCallbackHandle RegisterConnectionCallback(ConnectionCallbackFunction callbackFunction); // TODODB: Make a generic function for registering and unregistering all callbacks
	bool UnregisterConnectionCallback(ConnectionCallbackHandle handle);
	DisconnectionCallbackHandle RegisterDisconnectionCallback(DisconnectionCallbackFunction callbackFunction);
	bool UnregisterDisconnectionCallback(DisconnectionCallbackHandle handle);

	uint32_t GetConnectionCount();
	ConnectionInfo GetConnectionInfo(ConnectionID id);
	std::string GetAddressOfConnection(ConnectionID id);
	uint16_t GetPortOfConnection(ConnectionID id);

	bool IsValidIPv4Address(const char* ipv4String);
};