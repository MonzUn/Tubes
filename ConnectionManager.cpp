#include "ConnectionManager.h"
#include <thread>
#include <messaging/MessagingTypes.h>
#include "TubesMessageReplicator.h"
#include "TubesMessages.h"
#include "TubesUtility.h"
#include "TubesErrors.h"
#include "Connection.h"
#include "Communication.h"

#if PLATFORM != PLATFORM_WINDOWS
#include <arpa/inet.h>
#endif

#if PLATFORM == PLATFORM_WINDOWS
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == WSAEWOULDBLOCK )
#elif PLATFORM == PLATFORM_LINUX
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == EINPROGRESS )
#endif

#define CONNECTION_TIMEOUT_SECONDS 2 // TODODB: Make this a settable variable

using namespace TubesUtility;

void ConnectionManager::VerifyNewConnections( bool isHost, TubesMessageReplicator& replicator, const tVector<TubesMessage*> receivedMessages ) {
	for ( int i = 0; i < m_UnverifiedConnections.size(); ++i ) {
		switch ( m_UnverifiedConnections[i].second ) {

			case ConnectionState::NEW_IN : { // TODODB: Implement logic for checking so that the remote client really is a tubes client
				if ( isHost ) {
					ConnectionIDMessage idMessage = ConnectionIDMessage( m_NextConnectionID );
					Communication::SendTubesMessage( *m_UnverifiedConnections[i].first, idMessage, replicator );

					// TODODB: Call callback to let the external application know that there is a new connection

					m_Connections.emplace( m_NextConnectionID++, m_UnverifiedConnections[i].first );
					m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i-- );
				} else {
					assert( false ); // A client shouldn't receive incoming connections
				}
			} break;

			case ConnectionState::NEW_OUT : {
				if( !isHost ) {
					for ( int i = 0; i < receivedMessages.size(); ++i ) {
						if ( receivedMessages[i]->Type == Messages::CONNECTION_ID ) {
							ConnectionIDMessage* idMessage = static_cast<ConnectionIDMessage*>( receivedMessages[i] );

							// TODODB: Call callback to let the external application know that there is a new connection

							m_Connections.emplace( idMessage->ID, m_UnverifiedConnections[i].first );
							m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i-- );
						}
					}
				} else {
					assert( false ); // A host shouldn't initiate connections
				}
			} break;

			default: {
				assert( false ); // Connection is in an unhandled connection state
			} break;
				
		}
	}
}

void ConnectionManager::RequestConnection( const tString& address, Port port ) {
	std::thread connectionThread = std::thread( &ConnectionManager::Connect, this, address, port ); // TODODB: Use a thread pool
	connectionThread.detach();
}

void ConnectionManager::StartListener( Port port ) { // TODODB: Add check against duplicates

	Socket listeningSocket = static_cast<Socket>( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ); // Will be used to listen for incoming connections
	if ( listeningSocket == INVALID_SOCKET ) {
		LogErrorMessage( "Failed to set up listening socket" );
		return;
	}

	// Allow reuse of listening socket port
	char reuse = 1;
	setsockopt( listeningSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( char ) ); // TODODB: Check return value

	// Set up the sockaddr for the listenign socket
	sockaddr_in sockAddr;
	memset( &sockAddr, 0, sizeof( sockAddr ) );
	sockAddr.sin_family			= AF_INET;
	sockAddr.sin_addr.s_addr	= htonl( INADDR_ANY );
	sockAddr.sin_port			= htons( port );

	//Bind the listening socket object to an actual socket.
	if ( bind( listeningSocket, ( sockaddr* )&sockAddr, sizeof( sockAddr ) ) < 0 ) {
		LogErrorMessage( "Failed to bind listening socket" );
		return;
	}

	// Start listening for incoming connections
	if ( listen( listeningSocket, MAX_LISTENING_BACKLOG ) < 0 ) {
		LogErrorMessage( "Failed to start listening socket" );
		return;
	}

	LogInfoMessage( "Listening for incoming connections on port " + rToString( port ) );
	Listener* listener = pNew( Listener ); // TODODB: See if we can change stuff around to get this off the heap
	std::thread* thread = pNew( std::thread, &ConnectionManager::Listen, this, listeningSocket, listener->ShouldTerminate );
	listener->Thread = thread;
	listener->ListeningSocket = listeningSocket;
	
	m_ListenerMap.emplace( port, listener );
}

void ConnectionManager::StopAllListeners() {
	// Signal all listeners to stop
	for ( auto portListenerPair : m_ListenerMap ) {
		*portListenerPair.second->ShouldTerminate = true;
		ShutdownAndCloseSocket( portListenerPair.second->ListeningSocket );
	}

	// Join all listener threads (Should hopefully avoid blocks since they've all been signalled to stop previously)
	for ( auto portListenerPair : m_ListenerMap ) {
		portListenerPair.second->Thread->join();
		pDelete( portListenerPair.second );
	}

	m_ListenerMap.clear();
}

void ConnectionManager::Connect( const tString& address, Port port ) { // TODODB: Make this run on a separate thread to avoid blocking the main thread
	// Set up the socket
	Socket connectionSocket = static_cast<int64_t>( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ); // Adress Family = INET and the protocol to be used is TCP
	if ( connectionSocket <= 0 ) {
		LogErrorMessage( "Failed to create socket" );
		return;
	}

	Connection* connection = pNew( Connection, connectionSocket, address, port );

	connection->SetBlockingMode( false );

	// Set up timeout variables
	timeval timeOut;
	timeOut.tv_sec	= CONNECTION_TIMEOUT_SECONDS;
	timeOut.tv_usec = 0;

	fd_set set;
	FD_ZERO( &set );
	FD_SET( connection->socket, &set );

	// Attempt to connect
	LogInfoMessage( "Attempting to connect to " + address );

	int result;
	if ( ( result = connect( connection->socket, reinterpret_cast<sockaddr*>( &connection->sockaddr ), sizeof( sockaddr_in ) ) ) == INVALID_SOCKET ) {
		if ( !SHOULD_WAIT_FOR_TIMEOUT ) {
			pDelete( connection );
			return;
		}
	}
	result = select( static_cast<int>( connection-> socket + 1 ), NULL, &set, NULL, &timeOut );

	// Check result of the connection
	if ( result == 0 ) {
		LogInfoMessage( "Connection attempt to " + address + " timed out" );
		pDelete( connection );
		return;
	} else if ( result < 0 ) {
		LogErrorMessage( "Connection attempt to " + address + " failed" );
		pDelete( connection );
		return;
	}

	connection->SetNoDelay();

	LogInfoMessage( "Connection attempt to " + address + " was successfull!" );
	m_UnverifiedConnectionsLock.lock();
	m_UnverifiedConnections.push_back( std::pair<Connection*, ConnectionState>( connection, NEW_OUT ) );
	m_UnverifiedConnectionsLock.unlock();
}

void ConnectionManager::Listen( Socket listeningsSocket, std::atomic_bool* shouldTerminate ) {
	do {
		Socket					incomingConnectionSocket;
		sockaddr_in				incomingConnectionInfo;
		socklen_t				incomingConnectionInfoLength = sizeof( incomingConnectionInfo );

		// Wait for a connection or fetch one from the backlog
		incomingConnectionSocket = static_cast<Socket>( accept( listeningsSocket, reinterpret_cast<sockaddr*>( &incomingConnectionInfo ), &incomingConnectionInfoLength ) ); // Blocking
		if ( incomingConnectionSocket != INVALID_SOCKET ) {
			Connection* connection = pNew( Connection, incomingConnectionSocket, incomingConnectionInfo );
			m_UnverifiedConnectionsLock.lock();
			m_UnverifiedConnections.push_back( std::pair<Connection*, ConnectionState>( connection, NEW_IN ) );
			m_UnverifiedConnectionsLock.unlock();
		} else {
			int error = GET_NETWORK_ERROR;
			if ( error != TUBES_EINTR ) // The socket was killed on purpose
				LogErrorMessage( "Incoming connection attempt failed" );
		}
	} while ( !*shouldTerminate );
}

void ConnectionManager::ShutdownAndCloseSocket( Socket socket ) {
	int result;
#if PLATFORM == PLATFORM_WINDOWS
	result = shutdown( socket, SD_BOTH );
#else
	result = shutdown( socket, SHUT_RDWR );
#endif
	if ( result != 0 )
		LogErrorMessage( "Failed to shut down socket" );

#if PLATFORM == PLATFORM_WINDOWS
	result = closesocket( socket );
#else
	result = close( socket );
#endif
	if ( result != 0 )
		LogErrorMessage( "Failed to close socket" );

	socket = INVALID_SOCKET;
}

Connection* ConnectionManager::GetConnection( ConnectionID connectionID ) const {
	Connection* toReturn = nullptr;
	if ( m_Connections.find( connectionID ) != m_Connections.end() ) {
		toReturn = m_Connections.at( connectionID );
	} else {
		LogWarningMessage( "Attempted to fetch unexsisting connection (ID = " + rToString( connectionID ) + " )" );
	}

	return toReturn;
}

const pMap<ConnectionID, Connection*>& ConnectionManager::GetVerifiedConnections() const {
	return m_Connections;
}