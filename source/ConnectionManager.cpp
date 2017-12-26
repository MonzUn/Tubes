#include "ConnectionManager.h"
#include "interface/messaging/MessagingTypes.h"
#include "TubesMessageReplicator.h"
#include "TubesMessages.h"
#include "TubesUtility.h"
#include "TubesErrors.h"
#include "Connection.h"
#include "Communication.h"
#include <MUtilityLog.h>
#include <thread>
#include <cassert>

#if PLATFORM != PLATFORM_WINDOWS
#include <arpa/inet.h>
#include <unistd.h>
#endif

#if PLATFORM == PLATFORM_WINDOWS
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == WSAEWOULDBLOCK )
#elif PLATFORM == PLATFORM_LINUX
#define SHOULD_WAIT_FOR_TIMEOUT static_cast<bool>( GET_NETWORK_ERROR == EINPROGRESS )
#endif

#define TUBES_LOG_CATEGORY_CONNECTIONMANAGER "TubesConnectionManager"
#define CONNECTION_TIMEOUT_SECONDS 2 // TODODB: Make this a settable variable

using namespace TubesUtility;

ConnectionManager::~ConnectionManager()
{
	DisconnectAll();
}

void ConnectionManager::VerifyNewConnections( bool isHost, TubesMessageReplicator& replicator )
{
	for ( int i = 0; i < m_UnverifiedConnections.size(); ++i )
	{
		std::unordered_map<ReplicatorID, MessageReplicator*> replicatorMap; // TODODB: Create overload of Communication::Receive that takes only a single replicator
		replicatorMap.emplace( replicator.GetID(), &replicator );

		Connection*& connection = m_UnverifiedConnections[i].first;

		switch ( m_UnverifiedConnections[i].second )
		{
			case ConnectionState::NEW_IN : // TODODB: Implement logic for checking so that the remote client really is a tubes client
				if ( isHost )
				{
					ConnectionID connectionID = m_NextConnectionID++;
					ConnectionIDMessage idMessage = ConnectionIDMessage( connectionID );
					Communication::SendTubesMessage( *connection, idMessage, replicator );

					m_Connections.emplace( connectionID, connection);
					m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i-- );
					m_ConnectionCallbacks.TriggerCallbacks( idMessage.ID );

					MLOG_INFO( "An incoming connection with destination " + TubesUtility::AddressToIPv4String(m_Connections.at(connectionID)->address) + " was accepted", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
				}
				else
				{
					assert( false && "TUBES: Received incoming connection while in client mode" );
				}
				break;

			case ConnectionState::NEW_OUT:
				if ( !isHost )
				{
					Message* message = nullptr;
					ReceiveResult result;
					result = Communication::Receive( *connection, replicatorMap, message );
					switch( result )
					{
						case ReceiveResult::Fullmessage:
						{
							if ( message->Type == TubesMessages::CONNECTION_ID )
							{
								ConnectionIDMessage* idMessage = static_cast<ConnectionIDMessage*>( message );

								m_Connections.emplace( idMessage->ID, connection);
								m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i-- );
								m_ConnectionCallbacks.TriggerCallbacks( idMessage->ID );

								MLOG_INFO( "An outgoing connection with destination " + TubesUtility::AddressToIPv4String( m_Connections.at( idMessage->ID )->address ) + " was accepted", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
								free( message );
							}
							else
							{
								MLOG_WARNING( "Received an unexpected message type while verifying socket; message type = " << message->Type, TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
								free( message );
							}
						} break;

						case ReceiveResult::GracefulDisconnect:
						case ReceiveResult::ForcefulDisconnect:
						{
							MLOG_INFO( "An unverified outgoing connection with destination " + TubesUtility::AddressToIPv4String( connection->address ) + " was disconnected during handshake", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

							ShutdownAndCloseSocket( connection->socket );
							delete connection;
							m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i-- );
						} break;

						case ReceiveResult::Empty:
						case ReceiveResult::PartialMessage:
						case ReceiveResult::Error:
						default:
							break;
					} 
				}
				else
				{
					assert( false && "TUBES: Attempted to initiate connection while in host mode" );
				}
				break;

			default:
				assert( false && "TUBES: A connection is in an unhandled connection state" );
			break;
		}
	}
}

void ConnectionManager::RequestConnection( const std::string& address, Port port )
{
	std::thread connectionThread = std::thread( &ConnectionManager::Connect, this, address, port ); // TODODB: Use a thread pool
	connectionThread.detach();
}

void ConnectionManager::DisconnectConnection( ConnectionID connectionID )
{
	auto connectionIterator = m_Connections.find( connectionID );
	if ( connectionIterator != m_Connections.end() )
	{
		Connection* connection = m_Connections.at( connectionID );
		ShutdownAndCloseSocket( connection->socket );
		MLOG_INFO( "A connection with destination " + TubesUtility::AddressToIPv4String( connection->address ) + " has been disconnected", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

		delete connection;
		m_Connections.erase( connectionIterator );

		m_DisconnectionCallbacks.TriggerCallbacks( connectionID );
	}
	else
		MLOG_WARNING( "Attempted to disconnect socket with id: " << connectionID + " but no socket with that ID was found", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
}

void ConnectionManager::DisconnectAll()
{
	for ( auto& connectionAndState : m_UnverifiedConnections )
	{
		ShutdownAndCloseSocket( connectionAndState.first->socket );
		MLOG_INFO( "An unverified connection with destination " + TubesUtility::AddressToIPv4String(connectionAndState.first->address) + " has been disconnected", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

		delete connectionAndState.first;
	}
	m_UnverifiedConnections.clear();

	for ( auto& idAndConnection : m_Connections )
	{
		ShutdownAndCloseSocket( idAndConnection.second->socket );
		MLOG_INFO( "A connection with destination " + TubesUtility::AddressToIPv4String(idAndConnection.second->address) + " has been disconnected", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

		delete idAndConnection.second;

		m_DisconnectionCallbacks.TriggerCallbacks( idAndConnection.first );
	}
	m_Connections.clear();
}

void ConnectionManager::StartListener( Port port ) // TODODB: Add check against duplicates
{
	Socket listeningSocket = static_cast<Socket>( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ); // Will be used to listen for incoming connections
	if ( listeningSocket == INVALID_SOCKET )
	{
		LogAPIErrorMessage( "Failed to set up listening socket", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
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
	if ( bind( listeningSocket, ( sockaddr* )&sockAddr, sizeof( sockAddr ) ) < 0 )
	{
		LogAPIErrorMessage( "Failed to bind listening socket", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
		return;
	}

	// Start listening for incoming connections
	if ( listen( listeningSocket, MAX_LISTENING_BACKLOG ) < 0 )
	{
		LogAPIErrorMessage("Failed to start listening socket", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
		return;
	}

	MLOG_INFO( "Listening for incoming connections on port " << port, TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
	Listener* listener = new Listener; // TODODB: See if we can change stuff around to get this off the heap
	std::thread* thread = new std::thread( &ConnectionManager::Listen, this, listeningSocket, listener->ShouldTerminate );
	listener->Thread = thread;
	listener->ListeningSocket = listeningSocket;
	
	m_ListenerMap.emplace( port, listener );
}

void ConnectionManager::StopAllListeners()
{
	// Signal all listeners to stop
	for ( auto portListenerPair : m_ListenerMap )
	{
		*portListenerPair.second->ShouldTerminate = true;
		ShutdownAndCloseSocket( portListenerPair.second->ListeningSocket );
	}

	// Join all listener threads (Should hopefully avoid blocks since they've all been signalled to stop previously)
	for ( auto portListenerPair : m_ListenerMap )
	{
		portListenerPair.second->Thread->join();
		delete portListenerPair.second;
	}

	m_ListenerMap.clear();
}

ConnectionCallbackHandle ConnectionManager::RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction )
{
	return m_ConnectionCallbacks.RegisterCallback( callbackFunction );
}

bool ConnectionManager::UnregisterConnectionCallback( ConnectionCallbackHandle handle )
{
	return m_ConnectionCallbacks.UnregisterCallback( handle );
}

DisconnectionCallbackHandle ConnectionManager::RegisterDisconnectionCallback( DisconnectionCallbackFunction callbackFunction )
{
	return m_DisconnectionCallbacks.RegisterCallback( callbackFunction );
}

bool ConnectionManager::UnregisterDisconnectionCallback( DisconnectionCallbackHandle handle )
{
	return m_DisconnectionCallbacks.UnregisterCallback( handle );
}

void ConnectionManager::Connect( const std::string& address, Port port ) // TODODB: Make this run on a separate thread to avoid blocking the main thread
{
	// Set up the socket
	Socket connectionSocket = static_cast<int64_t>( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ); // Adress Family = INET and the protocol to be used is TCP
	if ( connectionSocket <= 0 )
	{
		MLOG_ERROR( "Failed to create socket", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
		return;
	}

	Connection* connection = new Connection( connectionSocket, address, port );

	connection->SetBlockingMode( false );

	// Set up timeout variables
	timeval timeOut;
	timeOut.tv_sec	= CONNECTION_TIMEOUT_SECONDS;
	timeOut.tv_usec = 0;

	fd_set set;
	FD_ZERO( &set );
	FD_SET( connection->socket, &set );

	// Attempt to connect
	MLOG_INFO( "Attempting to connect to " + address, TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

	int result;
	if ( ( result = connect( connection->socket, reinterpret_cast<sockaddr*>( &connection->sockaddr ), sizeof( sockaddr_in ) ) ) == INVALID_SOCKET )
	{
		if ( !SHOULD_WAIT_FOR_TIMEOUT )
		{
			delete connection;
			return;
		}
	}
	result = select( static_cast<int>( connection-> socket + 1 ), NULL, &set, NULL, &timeOut );

	// Check result of the connection
	if ( result == 0 )
	{
		MLOG_INFO( "Connection attempt to " + address + " timed out", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
		delete connection;
		return;
	}
	else if ( result < 0 )
	{
		LogAPIErrorMessage( "Connection attempt to " + address + " failed", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
		delete connection;
		return;
	}

	connection->SetNoDelay();

	MLOG_INFO( "Connection attempt to " + address + " was successfull!", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );
	m_UnverifiedConnectionsLock.lock();
	m_UnverifiedConnections.push_back( std::pair<Connection*, ConnectionState>( connection, NEW_OUT ) );
	m_UnverifiedConnectionsLock.unlock();
}

void ConnectionManager::Listen( Socket listeningsSocket, std::atomic_bool* shouldTerminate )
{
	do
	{
		Socket					incomingConnectionSocket;
		sockaddr_in				incomingConnectionInfo;
		socklen_t				incomingConnectionInfoLength = sizeof( incomingConnectionInfo );

		// Wait for a connection or fetch one from the backlog
		incomingConnectionSocket = static_cast<Socket>( accept( listeningsSocket, reinterpret_cast<sockaddr*>( &incomingConnectionInfo ), &incomingConnectionInfoLength ) ); // Blocking
		if ( incomingConnectionSocket != INVALID_SOCKET )
		{
			Connection* connection = new Connection( incomingConnectionSocket, incomingConnectionInfo );
			m_UnverifiedConnectionsLock.lock();
			connection->SetBlockingMode( false );
			connection->SetNoDelay();
			m_UnverifiedConnections.push_back( std::pair<Connection*, ConnectionState>( connection, NEW_IN ) );
			m_UnverifiedConnectionsLock.unlock();
		}
		else
		{
			int error = GET_NETWORK_ERROR;
			if (error != TUBES_EINTR) // The socket was killed on purpose
				LogAPIErrorMessage("Incoming connection attempt failed", TUBES_LOG_CATEGORY_CONNECTIONMANAGER);
		}
	} while ( !*shouldTerminate );
}

void ConnectionManager::ShutdownAndCloseSocket( Socket socket )
{
	int result;
#if PLATFORM == PLATFORM_WINDOWS
	result = shutdown( socket, SD_BOTH );
#else
	result = shutdown( socket, SHUT_RDWR );
#endif
	if (result != 0)
		LogAPIErrorMessage("Failed to shut down socket", TUBES_LOG_CATEGORY_CONNECTIONMANAGER);

#if PLATFORM == PLATFORM_WINDOWS
	result = closesocket( socket );
#else
	result = close( socket );
#endif
	if (result != 0)
		LogAPIErrorMessage( "Failed to close socket", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

	socket = INVALID_SOCKET;
}

Connection* ConnectionManager::GetConnection( ConnectionID connectionID ) const
{
	Connection* toReturn = nullptr;
	if (m_Connections.find(connectionID) != m_Connections.end())
		toReturn = m_Connections.at(connectionID);
	else
		MLOG_WARNING( "Attempted to fetch unexsisting connection (ID = " << connectionID + " )", TUBES_LOG_CATEGORY_CONNECTIONMANAGER );

	return toReturn;
}

const std::unordered_map<ConnectionID, Connection*>& ConnectionManager::GetVerifiedConnections() const
{
	return m_Connections;
}
