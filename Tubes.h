#pragma once
#include <messaging/MessagingTypes.h>
#include "TubesLibraryDefine.h"
#include "ConnectionManager.h"

class MessageReplicator;
class TubesMessageReplicator;

class Tubes { // TODODB: Create interface
public:
	TUBES_API bool Initialize();
	TUBES_API void Shutdown();
	TUBES_API void Update();

	TUBES_API void RequestConnection( const tString& address, uint16_t port );
	TUBES_API void StartListener( uint16_t port );
	TUBES_API void StopAllListeners();

	TUBES_API bool GetHostFlag() const;

	TUBES_API void SetHostFlag( bool isHost );

private:
	ConnectionManager m_ConnectionManager;

	pMap<ReplicatorID, MessageReplicator*>	m_ReplicatorReferences;
	TubesMessageReplicator*					m_TubesMessageReplicator;

	tVector<TubesMessage*>					m_ReceivedTubesMessages;

	bool m_Initialized	= false;
	bool m_HostFlag		= false;
};