#include "Tubes.h"
#include <utility/PlatformDefinitions.h>
#include "TubesUtility.h"
#include "TubesMessageReplicator.h"
#include "Communication.h"

#if PLATFORM == PLATFORM_WINDOWS
	#include <utility/RetardedWindowsIncludes.h>
	#pragma comment( lib, "Ws2_32.lib" ) // TODODB: See if this can be done through cmake instead
#elif PLATFORM == PLATFORM_LINUX
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
#endif

bool Tubes::Initialize() { // TODODB: Make sure this cannot be called if the isntance is already initialized
#if PLATFORM == PLATFORM_WINDOWS
	WSADATA wsaData;
	m_Initialized = WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == NO_ERROR; // Initialize WSA version 2.2
	if ( !m_Initialized ) {
			LogErrorMessage( "Failed to initialize Tubes since WSAStartup failed" );	
	}
#else 
	m_Initialized = true;
#endif

	if ( m_Initialized ) {
		m_TubesMessageReplicator = pNew( TubesMessageReplicator );
		m_ReplicatorReferences.emplace( m_TubesMessageReplicator->GetID(), m_TubesMessageReplicator );

		LogInfoMessage( "Tubes was successfully initialized" );
	}

	return m_Initialized;
}

void Tubes::Shutdown() {
	if ( m_Initialized ) {
		m_ConnectionManager.StopAllListeners();

		#if PLATFORM == PLATFORM_WINDOWS
			WSACleanup();
		#endif

		pDelete( m_TubesMessageReplicator );
		m_ReplicatorReferences.clear();

		LogInfoMessage( "Tubes has been shut down" );
	} else {
		LogWarningMessage( "Attempted to shut down uninitialized isntance of Tubes" );
	}

	m_Initialized = false;
}

void Tubes::Update() {
	if ( m_Initialized ) {
		m_ConnectionManager.VerifyNewConnections( m_HostFlag, *m_TubesMessageReplicator, m_ReceivedTubesMessages );
	} else {
		LogWarningMessage( "Attempted to update uninitialized instance of Tubes" );
	}
}

void Tubes::SendToAll( const Message* message ) {
	if ( m_Initialized ) {
		if ( m_ReplicatorReferences.find( message->Replicator_ID ) != m_ReplicatorReferences.end() ) {
			const pMap<ConnectionID, Connection*>& connections = m_ConnectionManager.GetAllConnections();
			for ( auto& idAndConnection : connections ) {
				Communication::SendTubesMessage( *idAndConnection.second, *message, *m_ReplicatorReferences.at( message->Replicator_ID ) );
			}
		} else {
			LogWarningMessage( "Attempted to send message for which no replicator has been registered. Replicator ID = " + rToString( message->Replicator_ID ) );
		}
	} else {
		LogWarningMessage( "Attempted to send using an uninitialized instance of Tubes" );
	}
}

void Tubes::Receive( tVector<Message*>& outMessages ) {
	if ( m_Initialized ) {
		const pMap<ConnectionID, Connection*>& connections = m_ConnectionManager.GetAllConnections();
		for ( auto& idAndConnection : connections ) {
			Message* message;
			while ( message = Communication::Receive( *idAndConnection.second, m_ReplicatorReferences ) ) {
				if ( message->Replicator_ID == TubesMessageReplicator::TubesMessageReplicatorID ) {
					m_ReceivedTubesMessages.push_back( reinterpret_cast<TubesMessage*>( message ) ); // We know that this is a tubes message
				} else {
					outMessages.push_back( message );
				}
			}
		}
	} else {
		LogWarningMessage( "Attempted to receive using an uninitialized instance of Tubes" );
	}
}

void Tubes::RequestConnection( const tString& address, uint16_t port ) {
	if ( m_Initialized ) {
		m_ConnectionManager.RequestConnection( address, port );
	} else {
		LogWarningMessage( "Attempted to request a connection although the Tubes instance is uninitialized" );
	}
}

void Tubes::StartListener( uint16_t port ) {
	if ( m_Initialized ) {
		m_ConnectionManager.StartListener( port );
	} else {
		LogWarningMessage( "Attempted to start listening on port " + rToString( port ) + " although the tubes instance is uninitialized" );
	}
}

void Tubes::StopAllListeners() {
	if ( m_Initialized ) {
		m_ConnectionManager.StopAllListeners();
	} else {
		LogWarningMessage( "Attempted to stop all listeners although the tubes instance is uninitialized" );
	}
}

bool Tubes::GetHostFlag() const {
	return m_HostFlag;
}

void Tubes::SetHostFlag( bool newHostFlag ) {
	m_HostFlag = newHostFlag;
}