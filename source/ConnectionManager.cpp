#include "ConnectionManager.h"
#include "Interface/Messaging/MessagingTypes.h"
#include "Interface/TubesSettings.h"
#include "Connection.h"
#include "Listener.h"
#include "TubesErrors.h"
#include "TubesMessageReplicator.h"
#include "TubesMessages.h"
#include "TubesUtility.h"
#include <MUtilityLog.h>
#include <cassert>
#include <thread>

#if PLATFORM != PLATFORM_WINDOWS
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define LOG_CATEGORY_CONNECTION_MANAGER "TubesConnectionManager"

using namespace TubesUtility;

ConnectionManager::~ConnectionManager()
{
	DisconnectAll();
}

void ConnectionManager::VerifyNewConnections( TubesMessageReplicator& replicator )
{
	std::vector<std::pair<Connection*, ConnectionState>> newConnections;
	for ( const auto& portAndListener : m_ListenerMap )
	{
		portAndListener.second->FetchAcceptedConnections( newConnections );
	}

	if (!Settings::AllowDuplicateConnections)
	{
		// Make sure that the new connection doesn't already exist
		for (int i = 0; i < newConnections.size(); ++i)
		{
			bool duplicate = false;
			Connection*& newConnection = newConnections[i].first;
			for (int j = 0; j < m_UnverifiedConnections.size(); ++j)
			{
				const Connection* existingConnection = m_UnverifiedConnections[j].first;
				if (newConnection->GetAddress() == existingConnection->GetAddress() && newConnection->GetPort() == existingConnection->GetPort())
				{
					duplicate = true;
					break;
				}
			}

			if (!duplicate)
			{
				for (const auto& idAndConnection : m_Connections)
				{
					const Connection* existingConnection = idAndConnection.second;
					if (newConnection->GetAddress() == existingConnection->GetAddress() && newConnection->GetPort() == existingConnection->GetPort())
					{
						duplicate = true;
						break;
					}
				}
			}

			if (duplicate)
			{
				MLOG_WARNING("An incoming connection with destination " << TubesUtility::AddressToIPv4String(newConnection->GetAddress()) << " was diesconnected since an identical connection already existed", LOG_CATEGORY_CONNECTION_MANAGER);
				newConnection->Disconnect();
				delete newConnection;
				newConnection = nullptr;
			}
			else
				m_UnverifiedConnections.push_back(newConnections[i]);
		}
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
			{
				ConnectionID connectionID = m_NextConnectionID++;
				ConnectionIDMessage idMessage = ConnectionIDMessage(connectionID);
				SendResult result = connection->SerializeAndSendMessage(idMessage, replicator);
				switch (result)
				{
				case SendResult::Disconnect:
				{
					connection->Disconnect();
					delete connection;
					m_UnverifiedConnections.erase(m_UnverifiedConnections.begin() + i);
					--i;
				} break;

				case SendResult::Sent:
				case SendResult::Queued:
				case SendResult::Error:
				default:
					break;
				}

				m_Connections.emplace(connectionID, connection);
				m_UnverifiedConnections.erase(m_UnverifiedConnections.begin() + i--);

				MLOG_INFO("An incoming connection with destination " + TubesUtility::AddressToIPv4String(m_Connections.at(connectionID)->GetAddress()) + " was accepted", LOG_CATEGORY_CONNECTION_MANAGER);
				m_ConnectionCallbacks.TriggerCallbacks(idMessage.ID);
			} break;

			case ConnectionState::NewOutgoing:
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

							MLOG_INFO( "An outgoing connection with destination " + TubesUtility::AddressToIPv4String( m_Connections.at( idMessage->ID )->GetAddress() ) + " was accepted", LOG_CATEGORY_CONNECTION_MANAGER);
							m_ConnectionCallbacks.TriggerCallbacks( idMessage->ID );
							free( message );
						}
						else
						{
							MLOG_WARNING( "Received an unexpected message type while verifying socket; message type = " << message->Type, LOG_CATEGORY_CONNECTION_MANAGER);
							free( message );
						}
					} break;

					case ReceiveResult::GracefulDisconnect:
					case ReceiveResult::ForcefulDisconnect:
					{
						MLOG_INFO( "An unverified outgoing connection with destination " + TubesUtility::AddressToIPv4String( connection->GetAddress() ) + " was disconnected during handshake", LOG_CATEGORY_CONNECTION_MANAGER);

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
			} break;

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

void ConnectionManager::Disconnect( ConnectionID connectionID ) // TODODB: Return boolean result
{
	auto& connectionIterator = m_Connections.find( connectionID );
	if ( connectionIterator != m_Connections.end() )
	{
		Connection* connection = m_Connections.at( connectionID );
		connection->Disconnect();
		MLOG_INFO( "A connection with destination " + TubesUtility::AddressToIPv4String( connection->GetAddress() ) + " has been disconnected", LOG_CATEGORY_CONNECTION_MANAGER);

		delete connection;
		m_Connections.erase( connectionIterator );

		m_DisconnectionCallbacks.TriggerCallbacks( connectionID );
	}
	else
		MLOG_WARNING( "Attempted to disconnect socket with id: " << connectionID + " but no socket with that ID was found", LOG_CATEGORY_CONNECTION_MANAGER);
}

void ConnectionManager::DisconnectAll()
{
	for (auto& connectionAndState = m_UnverifiedConnections.cbegin(); connectionAndState != m_UnverifiedConnections.cend();)
	{
		connectionAndState->first->Disconnect();
		MLOG_INFO( "An unverified connection with destination " + TubesUtility::AddressToIPv4String( connectionAndState->first->GetAddress() ) + " has been disconnected", LOG_CATEGORY_CONNECTION_MANAGER);

		delete connectionAndState->first;
	}
	m_UnverifiedConnections.clear();

	for ( auto& idAndConnection = m_Connections.cbegin(); idAndConnection != m_Connections.cend();)
	{
		ConnectionID connectionID = idAndConnection->first;

		idAndConnection->second->Disconnect();
		MLOG_INFO( "A connection with destination " + TubesUtility::AddressToIPv4String(idAndConnection->second->GetAddress() ) + " has been disconnected", LOG_CATEGORY_CONNECTION_MANAGER);

		delete idAndConnection->second;
		m_Connections.erase(idAndConnection++);

		m_DisconnectionCallbacks.TriggerCallbacks(connectionID);
	}
	m_Connections.clear();
}

bool ConnectionManager::StartListener(Port port)
{
	for (auto& portAndListener : m_ListenerMap)
	{
		if (portAndListener.first == port)
		{
			MLOG_WARNING("Attempted to start listening on a port that already has a listener; port = " << port, LOG_CATEGORY_CONNECTION_MANAGER);
			return false;
		}
	}

	Listener* listener = new Listener;
	bool result = listener->StartListening(port);
	if (result)
		m_ListenerMap.emplace(port, listener);

	return result;
}

bool ConnectionManager::StopListener(Port port)
{
	bool result = false;
	auto portAndListener = m_ListenerMap.find(port);
	if (portAndListener != m_ListenerMap.end())
	{
		portAndListener->second->StopListening();
		delete portAndListener->second;
		m_ListenerMap.erase(portAndListener);
		result = true;
	}
	else
		MLOG_WARNING("Attempted to stop nonexistent listener for port " << port, LOG_CATEGORY_CONNECTION_MANAGER);
	return result;
}

bool ConnectionManager::StopAllListeners()
{
	std::vector<Port> ports; // TODODB: MAke utility functions for getting keys or values from a map and putting them into a vector
	for (auto& portAndListener : m_ListenerMap)
	{
		ports.push_back(portAndListener.first);
	}

	for (int i = 0; i < ports.size(); ++i)
	{
		StopListener(ports[i]);
	}

	return m_ListenerMap.empty();
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
	Socket connectionSocket = static_cast<int64_t>( socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ); // Address Family = INET and the protocol to be used is TCP
	if ( connectionSocket <= 0 )
	{
		LogAPIErrorMessage( "Failed to create socket", LOG_CATEGORY_CONNECTION_MANAGER);
		return;
	}

	Connection* connection = new Connection( connectionSocket, address, port );
	connection->SetBlockingMode( false );
	if (connection->Connect())
	{
		connection->SetNoDelay(true);

		MLOG_INFO( "Connection attempt to " + address + " was successful!", LOG_CATEGORY_CONNECTION_MANAGER);
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
		MLOG_WARNING( "Attempted to fetch nonexistent connection (ID = " << connectionID + " )", LOG_CATEGORY_CONNECTION_MANAGER);

	return toReturn;
}

const std::unordered_map<ConnectionID, Connection*>& ConnectionManager::GetVerifiedConnections() const
{
	return m_Connections;
}

std::string ConnectionManager::GetAddressOfConnection(ConnectionID connectionID) const
{
	std::string toReturn = "";
	if (m_Connections.find(connectionID) != m_Connections.end())
	{
		toReturn = TubesUtility::AddressToIPv4String(m_Connections.at(connectionID)->GetAddress());
	}
	else
		MLOG_WARNING("Attempted to get address of nonexistent connection (ID = " << connectionID + " )", LOG_CATEGORY_CONNECTION_MANAGER);

	return toReturn;
}

Port ConnectionManager::GetPortOfConnection(ConnectionID connectionID) const
{
	Port toReturn = TUBES_PORT_ANY;
	if (m_Connections.find(connectionID) != m_Connections.end())
	{
		toReturn = m_Connections.at(connectionID)->GetPort();
	}
	else
		MLOG_WARNING("Attempted to get address of nonexistent connection (ID = " << connectionID + " )", LOG_CATEGORY_CONNECTION_MANAGER);

	return toReturn;
}