#include "ConnectionManager.h"
#include "interface/messaging/MessagingTypes.h"
#include "Connection.h"
#include "Listener.h"
#include "TubesMessageReplicator.h"
#include "TubesMessages.h"
#include "TubesUtility.h"
#include "TubesErrors.h"
#include <MUtilityLog.h>
#include <cassert>
#include <thread>

#if PLATFORM != PLATFORM_WINDOWS
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define TUBES_LOG_CATEGORY_CONNECTION_MANAGER "TubesConnectionManager"

using namespace TubesUtility;

ConnectionManager::~ConnectionManager()
{
	DisconnectAll();
}

void ConnectionManager::VerifyNewConnections( bool isHost, TubesMessageReplicator& replicator )
{
	for (auto& portAndListener : m_ListenerMap)
	{
		portAndListener.second->FetchAcceptedConnections( m_UnverifiedConnections );
	}

	for ( int i = 0; i < m_UnverifiedConnections.size(); ++i )
	{
		std::unordered_map<ReplicatorID, MessageReplicator*> replicatorMap; // TODODB: Create overload of Receive() that takes only a single replicator
		replicatorMap.emplace( replicator.GetID(), &replicator );

		Connection*& connection = m_UnverifiedConnections[i].first;

		SendResult sendResult = connection->SendQueuedMessages();
		if ( sendResult == SendResult::Disconnect )
		{
			connection->Disconnect();
			delete connection;
			m_UnverifiedConnections.erase(m_UnverifiedConnections.begin() + i);
			--i;
		}

		// Perform verification
		switch ( m_UnverifiedConnections[i].second )
		{
			case ConnectionState::NewIncoming: // TODODB: Implement logic for checking so that the remote client really is a tubes client
				if ( isHost )
				{
					ConnectionID connectionID = m_NextConnectionID++;
					ConnectionIDMessage idMessage = ConnectionIDMessage( connectionID );
					SendResult result = connection->SerializeAndSendMessage( idMessage, replicator );
					switch ( result )
					{
						case SendResult::Disconnect:
						{
							connection->Disconnect();
							delete connection;
							m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i );
							--i;
						} break;

						case SendResult::Sent:
						case SendResult::Queued:
						case SendResult::Error:
						default:
							break;
					}

					m_Connections.emplace( connectionID, connection);
					m_UnverifiedConnections.erase( m_UnverifiedConnections.begin() + i-- );
					m_ConnectionCallbacks.TriggerCallbacks( idMessage.ID );

					MLOG_INFO( "An incoming connection with destination " + TubesUtility::AddressToIPv4String( m_Connections.at( connectionID)->GetAddress() ) + " was accepted", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);
				}
				else
				{
					assert( false && "TUBES: Received incoming connection while in client mode" );
				}
				break;

			case ConnectionState::NewOutgoing:
				if ( !isHost )
				{
					Message* message = nullptr;
					ReceiveResult result;
					result = connection->Receive( replicatorMap, message );
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

								MLOG_INFO( "An outgoing connection with destination " + TubesUtility::AddressToIPv4String( m_Connections.at( idMessage->ID )->GetAddress() ) + " was accepted", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);
								free( message );
							}
							else
							{
								MLOG_WARNING( "Received an unexpected message type while verifying socket; message type = " << message->Type, TUBES_LOG_CATEGORY_CONNECTION_MANAGER);
								free( message );
							}
						} break;

						case ReceiveResult::GracefulDisconnect:
						case ReceiveResult::ForcefulDisconnect:
						{
							MLOG_INFO( "An unverified outgoing connection with destination " + TubesUtility::AddressToIPv4String( connection->GetAddress() ) + " was disconnected during handshake", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);

							connection->Disconnect();
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

void ConnectionManager::Disconnect( ConnectionID connectionID )
{
	auto connectionIterator = m_Connections.find( connectionID );
	if ( connectionIterator != m_Connections.end() )
	{
		Connection* connection = m_Connections.at( connectionID );
		connection->Disconnect();
		MLOG_INFO( "A connection with destination " + TubesUtility::AddressToIPv4String( connection->GetAddress() ) + " has been disconnected", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);

		delete connection;
		m_Connections.erase( connectionIterator );

		m_DisconnectionCallbacks.TriggerCallbacks( connectionID );
	}
	else
		MLOG_WARNING( "Attempted to disconnect socket with id: " << connectionID + " but no socket with that ID was found", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);
}

void ConnectionManager::DisconnectAll()
{
	for ( auto& connectionAndState : m_UnverifiedConnections )
	{
		connectionAndState.first->Disconnect();
		MLOG_INFO( "An unverified connection with destination " + TubesUtility::AddressToIPv4String( connectionAndState.first->GetAddress() ) + " has been disconnected", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);

		delete connectionAndState.first;
	}
	m_UnverifiedConnections.clear();

	for ( auto& idAndConnection : m_Connections )
	{
		idAndConnection.second->Disconnect();
		MLOG_INFO( "A connection with destination " + TubesUtility::AddressToIPv4String( idAndConnection.second->GetAddress() ) + " has been disconnected", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);

		delete idAndConnection.second;

		m_DisconnectionCallbacks.TriggerCallbacks( idAndConnection.first );
	}
	m_Connections.clear();
}

void ConnectionManager::StartListener( Port port ) // TODODB: Add check against duplicates
{
	Listener* listener = new Listener; // TODODB: See if we can change stuff around to get this off the heap
	if(listener->StartListening(port))
		m_ListenerMap.emplace( port, listener );
}

void ConnectionManager::StopAllListeners()
{
	for ( auto portListener : m_ListenerMap )
	{
		portListener.second->StopListening();
		delete portListener.second;
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
		MLOG_ERROR( "Failed to create socket", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);
		return;
	}

	Connection* connection = new Connection( connectionSocket, address, port );
	connection->SetBlockingMode( false );
	if (connection->Connect())
	{
		connection->SetNoDelay();

		MLOG_INFO( "Connection attempt to " + address + " was successful!", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);
		m_UnverifiedConnections.push_back( std::pair<Connection*, ConnectionState>( connection, ConnectionState::NewOutgoing) );
	}
	else
		delete connection;
}

Connection* ConnectionManager::GetConnection( ConnectionID connectionID ) const
{
	Connection* toReturn = nullptr;
	if (m_Connections.find(connectionID) != m_Connections.end())
		toReturn = m_Connections.at(connectionID);
	else
		MLOG_WARNING( "Attempted to fetch unexsisting connection (ID = " << connectionID + " )", TUBES_LOG_CATEGORY_CONNECTION_MANAGER);

	return toReturn;
}

const std::unordered_map<ConnectionID, Connection*>& ConnectionManager::GetVerifiedConnections() const
{
	return m_Connections;
}
