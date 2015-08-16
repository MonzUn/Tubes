#pragma once
#include <messaging/MessagingTypes.h>
#include "TubesLibraryDefine.h"
#include "TubesTypes.h" // Exposes the relevant types to the external application

class	MessageReplicator;
class	TubesMessageReplicator;
class	ConnectionManager;
struct	Message;
struct	TubesMessage;

class Tubes { // TODODB: Create interface
public:
	TUBES_API bool Initialize();
	TUBES_API void Shutdown();
	TUBES_API void Update();

	TUBES_API void SendToConnection( const Message* message, ConnectionID destinationConnectionID );
	TUBES_API void SendToAll( const Message* message, ConnectionID exception = INVALID_CONNECTION_ID );
	TUBES_API void Receive( tVector<Message*>& outMessages, tVector<ConnectionID>* outSenderIDs = nullptr );

	TUBES_API void RequestConnection( const tString& address, uint16_t port ); // TODODB: Can we use tubes specific typdefes in this interface (Want to use Port here)
	TUBES_API void StartListener( uint16_t port );
	TUBES_API void StopAllListeners();

	TUBES_API void RegisterReplicator( MessageReplicator* replicator );
	TUBES_API ConnectionCallbackHandle RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction );
	TUBES_API bool UnregisterConnectionCallback( ConnectionCallbackHandle handle );

	TUBES_API bool GetHostFlag() const;

	TUBES_API void SetHostFlag( bool isHost );

private:
	ConnectionManager*						m_ConnectionManager; // TODODB: WHY CAN'T THE LINKER FIND THE DTOR DEFINITION IF THIS IS PLACED ON THE STACK!?!?

	pMap<ReplicatorID, MessageReplicator*>	m_ReplicatorReferences;
	TubesMessageReplicator*					m_TubesMessageReplicator;

	tVector<TubesMessage*>					m_ReceivedTubesMessages;

	bool m_Initialized	= false;
	bool m_HostFlag		= false;
};