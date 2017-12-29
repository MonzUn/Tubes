#pragma once
#include "messaging/MessagingTypes.h"
#include "TubesTypes.h" // Exposes the relevant types to the external application

class	MessageReplicator;
struct	Message;

namespace Tubes
{
	bool Initialize();
	void Shutdown();
	void Update();

	void SendToConnection( const Message* message, ConnectionID destinationConnectionID );
	void SendToAll( const Message* message, ConnectionID exception = INVALID_CONNECTION_ID );
	void Receive( std::vector<Message*>& outMessages, std::vector<ConnectionID>* outSenderIDs = nullptr );

	void RequestConnection( const std::string& address, uint16_t port ); // TODODB: Can we use tubes specific typdefes in this interface (Want to use Port here)
	void StartListener( uint16_t port );
	void StopAllListeners();
	void Disconnect(ConnectionID connectionID);
	void DisconnectAll();

	void RegisterReplicator( MessageReplicator* replicator );

	ConnectionCallbackHandle RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction );
	bool UnregisterConnectionCallback( ConnectionCallbackHandle handle );
	DisconnectionCallbackHandle RegisterDisconnectionCallback( DisconnectionCallbackFunction callbackFunction );
	bool UnregisterDisconnectionCallback( DisconnectionCallbackHandle handle );

	bool GetHostFlag();
	void SetHostFlag( bool isHost );
};