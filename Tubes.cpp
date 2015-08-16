#include "Tubes.h"
#include <utility/PlatformDefinitions.h>
#include "TubesUtility.h"
#include "TubesMessageReplicator.h"
#include "Communication.h"
#include "ConnectionManager.h"

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
		m_ConnectionManager = pNew( ConnectionManager );
		m_TubesMessageReplicator = pNew( TubesMessageReplicator );
		m_ReplicatorReferences.emplace( m_TubesMessageReplicator->GetID(), m_TubesMessageReplicator );

		LogInfoMessage( "Tubes was successfully initialized" );
	}

	return m_Initialized;
}

void Tubes::Shutdown() {
	if ( m_Initialized ) {
		m_ConnectionManager->StopAllListeners();

		#if PLATFORM == PLATFORM_WINDOWS
			WSACleanup();
		#endif

		pDelete( m_ConnectionManager );

		pDelete( m_TubesMessageReplicator ); // TODODB: Make a utility cleanup function for this
		m_ReplicatorReferences.clear();

		for ( int i = 0; i < m_ReceivedTubesMessages.size(); ++i ) {
			tFree( m_ReceivedTubesMessages[i] );
		}
		m_ReceivedTubesMessages.clear();

		LogInfoMessage( "Tubes has been shut down" );
	} else {
		LogWarningMessage( "Attempted to shut down uninitialized isntance of Tubes" );
	}

	m_Initialized = false;
}

void Tubes::Update() {
	if ( m_Initialized ) {
		m_ConnectionManager->VerifyNewConnections( m_HostFlag, *m_TubesMessageReplicator );
	} else {
		LogErrorMessage( "Attempted to update uninitialized instance of Tubes" );
	}
}

void Tubes::SendToConnection( const Message* message, ConnectionID destinationConnectionID ) {
	if ( m_Initialized ) {
		Connection* connection = m_ConnectionManager->GetConnection( destinationConnectionID );
		if ( connection != nullptr ) {
			if ( m_ReplicatorReferences.find( message->Replicator_ID ) != m_ReplicatorReferences.end() ) {
				Communication::SendTubesMessage( *connection, *message, *m_ReplicatorReferences.at( message->Replicator_ID ) );
			} else {
				LogErrorMessage( "Attempted to send message for which no replicator has been registered. Replicator ID = " + rToString( message->Replicator_ID ) );
			}
		} else {
			LogErrorMessage( "Failed to find requested connection while sending (Requested ID = " + rToString( destinationConnectionID ) + " )" );
		}
	} else {
		LogErrorMessage( "Attempted to send using an uninitialized instance of Tubes" );
	}
}

void Tubes::SendToAll( const Message* message, ConnectionID exception ) {
	if ( m_Initialized ) {
		if ( m_ReplicatorReferences.find( message->Replicator_ID ) != m_ReplicatorReferences.end() ) {
			const pMap<ConnectionID, Connection*>& connections = m_ConnectionManager->GetVerifiedConnections();

#if TUBES_DEBUG == 1
			if ( exception != INVALID_CONNECTION_ID ) {
				bool exceptionExists = false;
				for ( auto& idAndConnection : connections ) {
					if ( idAndConnection.first == exception ) {
						exceptionExists = true;
					}
				}

				if ( !exceptionExists ) {
					LogWarningMessage( "The excepted connectionID supplied to SendToAll does not exist" );
				}
			}
#endif
			for ( auto& idAndConnection : connections ) {
				if ( idAndConnection.first != exception ) {
					Communication::SendTubesMessage( *idAndConnection.second, *message, *m_ReplicatorReferences.at( message->Replicator_ID ) );
				}
			}
		} else {
			LogErrorMessage( "Attempted to send message for which no replicator has been registered. Replicator ID = " + rToString( message->Replicator_ID ) );
		}
	} else {
		LogErrorMessage( "Attempted to send using an uninitialized instance of Tubes" );
	}
}

void Tubes::Receive( tVector<Message*>& outMessages, tVector<ConnectionID>* outSenderIDs ) {
	if ( m_Initialized ) {
		const pMap<ConnectionID, Connection*>& connections = m_ConnectionManager->GetVerifiedConnections();
		for ( auto& idAndConnection : connections ) {
			Message* message;
			while ( (message = Communication::Receive( *idAndConnection.second, m_ReplicatorReferences ) ) != nullptr ) {
				if ( message->Replicator_ID == TubesMessageReplicator::TubesMessageReplicatorID ) {
					m_ReceivedTubesMessages.push_back( reinterpret_cast<TubesMessage*>( message ) ); // We know that this is a tubes message
				} else {
					outMessages.push_back( message );
					if ( outSenderIDs ) {
						outSenderIDs->push_back( idAndConnection.first );
					}
				}
			}
		}
	} else {
		LogErrorMessage( "Attempted to receive using an uninitialized instance of Tubes" );
	}
}

void Tubes::RequestConnection( const tString& address, uint16_t port ) {
	if ( m_Initialized ) {
		m_ConnectionManager->RequestConnection( address, port );
	} else {
		LogErrorMessage( "Attempted to request a connection although the Tubes instance is uninitialized" );
	}
}

void Tubes::StartListener( uint16_t port ) {
	if ( m_Initialized ) {
		m_ConnectionManager->StartListener( port );
	} else {
		LogErrorMessage( "Attempted to start listening on port " + rToString( port ) + " although the tubes instance is uninitialized" );
	}
}

void Tubes::StopAllListeners() {
	if ( m_Initialized ) {
		m_ConnectionManager->StopAllListeners();
	} else {
		LogErrorMessage( "Attempted to stop all listeners although the tubes instance is uninitialized" );
	}
}

void Tubes::RegisterReplicator( MessageReplicator* replicator ) { // TODODB: Add unregistration function
	m_ReplicatorReferences.emplace( replicator->GetID(), replicator ); // TODODB: Add error checking (Nullptr and duplicates)
}

ConnectionCallbackHandle Tubes::RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction ) {
	return m_ConnectionManager->RegisterConnectionCallback( callbackFunction );
}

bool Tubes::UnregisterConnectionCallback( ConnectionCallbackHandle handle ) {
	return m_ConnectionManager->UnregisterConnectionCallback( handle );
}

bool Tubes::GetHostFlag() const {
	return m_HostFlag;
}

void Tubes::SetHostFlag( bool newHostFlag ) {
	m_HostFlag = newHostFlag;
}