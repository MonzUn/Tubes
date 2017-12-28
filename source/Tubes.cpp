#include "interface/Tubes.h"
#include <MUtilityPlatformDefinitions.h>
#include "TubesUtility.h"
#include "TubesMessageBase.h"
#include "TubesMessageReplicator.h"
#include "Communication.h"
#include "ConnectionManager.h"
#include <MUtilityLog.h>

#if PLATFORM == PLATFORM_WINDOWS
	#include <MUtilityWindowsInclude.h>
	#pragma comment( lib, "Ws2_32.lib" ) // TODODB: See if this can be done through cmake instead
#elif PLATFORM == PLATFORM_LINUX
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
#endif

#define TUBES_LOG_CATEGORY_GENERAL "Tubes"

ConnectionManager*	m_ConnectionManager; // TODODB: Create an internal header to structure these variables and functions

std::unordered_map<ReplicatorID, MessageReplicator*>	m_ReplicatorReferences;
TubesMessageReplicator*									m_TubesMessageReplicator;

std::vector<TubesMessage*> m_ReceivedTubesMessages;

bool m_Initialized = false;
bool m_HostFlag = false;

bool SendQueuedMessages( Connection& connection ) ;

bool Tubes::Initialize() // TODODB: Make sure this cannot be called if the isntance is already initialized
{ 
	if ( !m_Initialized )
	{
#if PLATFORM == PLATFORM_WINDOWS
		WSADATA wsaData;
		m_Initialized = WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == NO_ERROR; // Initialize WSA version 2.2
		if ( !m_Initialized )
			LogAPIErrorMessage( "Failed to initialize Tubes since WSAStartup failed", TUBES_LOG_CATEGORY_GENERAL );
#else 
		m_Initialized = true;
#endif

		if ( m_Initialized )
		{
			m_ConnectionManager = new ConnectionManager;
			m_TubesMessageReplicator = new TubesMessageReplicator;
			m_ReplicatorReferences.emplace( m_TubesMessageReplicator->GetID(), m_TubesMessageReplicator );

			MLOG_INFO( "Tubes initialized successfully", TUBES_LOG_CATEGORY_GENERAL );
		}
	}
	else
		MLOG_WARNING( "Attempted to initialize an already initialized instance of Tubes", TUBES_LOG_CATEGORY_GENERAL );

	return m_Initialized;
}

void Tubes::Shutdown()
{
	if ( m_Initialized )
	{
		m_ConnectionManager->StopAllListeners();
		m_ConnectionManager->DisconnectAll();

		#if PLATFORM == PLATFORM_WINDOWS
			WSACleanup();
		#endif

		delete m_ConnectionManager;

		delete m_TubesMessageReplicator; // TODODB: Make a utility cleanup function for this
		m_ReplicatorReferences.clear();

		for ( int i = 0; i < m_ReceivedTubesMessages.size(); ++i )
		{
			free( m_ReceivedTubesMessages[i] );
		}
		m_ReceivedTubesMessages.clear();

		MLOG_INFO( "Tubes has been shut down", TUBES_LOG_CATEGORY_GENERAL );
	}
	else
		MLOG_WARNING( "Attempted to shut down uninitialized isntance of Tubes", TUBES_LOG_CATEGORY_GENERAL );

	m_Initialized = false;
}

void Tubes::Update()
{
	if ( m_Initialized )
	{
		m_ConnectionManager->VerifyNewConnections( m_HostFlag, *m_TubesMessageReplicator );

		// Send queued messages
		std::vector<ConnectionID> toDisconnect;
		for ( auto& idAndConnection : m_ConnectionManager->GetVerifiedConnections() )
		{
			if ( !SendQueuedMessages( *idAndConnection.second ) )
			{
				// The connection is no longer in a usable state
				toDisconnect.push_back( idAndConnection.first );
				continue;
			}
		}

		for ( int i = 0; i < toDisconnect.size(); ++i )
		{
			m_ConnectionManager->Disconnect( toDisconnect[i] );
		}
	}
	else
		MLOG_ERROR( "Attempted to update uninitialized instance of Tubes", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::SendToConnection( const Message* message, ConnectionID destinationConnectionID )
{
	if ( m_Initialized )
	{
		Connection* connection = m_ConnectionManager->GetConnection( destinationConnectionID );
		if ( connection != nullptr )
		{
			// Attempt to send queued messages first
			if ( !SendQueuedMessages( *connection ) )
			{
				// The connection is no longer in a usable state
				m_ConnectionManager->Disconnect( destinationConnectionID );
				return;
			}

			if ( m_ReplicatorReferences.find( message->Replicator_ID ) != m_ReplicatorReferences.end() )
			{
				SendResult result = Communication::SerializeAndSendMessage( *connection, *message, *m_ReplicatorReferences.at( message->Replicator_ID ));
				switch ( result )
				{
					case SendResult::Disconnect:
					{
						m_ConnectionManager->Disconnect( destinationConnectionID );
					} break;

					case SendResult::Sent:
					case SendResult::Queued:
					case SendResult::Error:
					default:
						break;
				}
			}
			else
				MLOG_ERROR( "Attempted to send message for which no replicator has been registered. Replicator ID = " << message->Replicator_ID, TUBES_LOG_CATEGORY_GENERAL );
		}
		else
			MLOG_ERROR( "Failed to find requested connection while sending (Requested ID = " << destinationConnectionID + " )", TUBES_LOG_CATEGORY_GENERAL);
	}
	else
		MLOG_ERROR( "Attempted to send using an uninitialized instance of Tubes", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::SendToAll( const Message* message, ConnectionID exception )
{
	if ( m_Initialized )
	{
		if ( m_ReplicatorReferences.find( message->Replicator_ID ) != m_ReplicatorReferences.end() )
		{
			const std::unordered_map<ConnectionID, Connection*>& connections = m_ConnectionManager->GetVerifiedConnections();

#if TUBES_DEBUG == 1
			if ( exception != INVALID_CONNECTION_ID )
			{
				bool exceptionExists = false;
				for ( auto& idAndConnection : connections )
				{
					if ( idAndConnection.first == exception )
						exceptionExists = true;
				}

				if ( !exceptionExists )
					MLOG_WARNING( "The excepted connectionID supplied to SendToAll does not exist", TUBES_LOG_CATEGORY_GENERAL );
			}
#endif
			std::vector<ConnectionID> toDisconnect; // TODODB: Find a better way to disconnect connections while iterating over the connection map
			for ( auto& idAndConnection : connections )
			{
				if ( idAndConnection.first != exception )
				{
					// Attempt to send queued messages first
					if (!SendQueuedMessages( *idAndConnection.second ) )
					{
						// The connection is no longer in a usable state
						toDisconnect.push_back( idAndConnection.first );
						continue; 
					}
					
					SendResult result = Communication::SerializeAndSendMessage( *idAndConnection.second, *message, *m_ReplicatorReferences.at( message->Replicator_ID ) );
					switch ( result )
					{
						case SendResult::Disconnect:
						{
							toDisconnect.push_back( idAndConnection.first );
						} break;

						case SendResult::Sent:
						case SendResult::Queued:
						case SendResult::Error:
						default:
							break;
					}
				}
			}

			for (int i = 0; i < toDisconnect.size(); ++i)
			{
				m_ConnectionManager->Disconnect( toDisconnect[i] );
			}
		}
		else
			MLOG_ERROR("Attempted to send message for which no replicator has been registered. Replicator ID = " << message->Replicator_ID, TUBES_LOG_CATEGORY_GENERAL);
	}
	else
		MLOG_ERROR( "Attempted to send using an uninitialized instance of Tubes", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::Receive( std::vector<Message*>& outMessages, std::vector<ConnectionID>* outSenderIDs )
{
	if ( m_Initialized )
	{
		std::vector<ConnectionID> toDisconnect; // TODODB: Find a better way to disconnect connections while iterating over the connection map
		const std::unordered_map<ConnectionID, Connection*>& connections = m_ConnectionManager->GetVerifiedConnections();
		for ( auto& idAndConnection : connections )
		{
			bool disconnected = false;
			Message* message = nullptr;
			ReceiveResult result;
			do
			{
				result = Communication::Receive( *idAndConnection.second, m_ReplicatorReferences, message );
				switch ( result )
				{
					case ReceiveResult::Fullmessage:
					{
						if ( message->Replicator_ID == TubesMessageReplicator::TubesMessageReplicatorID )
						{
							m_ReceivedTubesMessages.push_back( reinterpret_cast<TubesMessage*>( message ) ); // We know that this is a tubes message
						}
						else
						{
							outMessages.push_back( message );
							if ( outSenderIDs )
								outSenderIDs->push_back( idAndConnection.first );
						}
					} break;

					case ReceiveResult::GracefulDisconnect:
					case ReceiveResult::ForcefulDisconnect:
					{
						toDisconnect.push_back( idAndConnection.first );
						disconnected = true;
					} break;

					case ReceiveResult::Empty:
					case ReceiveResult::PartialMessage:
					case ReceiveResult::Error:
					default:
						break;
				}
			} while ( result == ReceiveResult::Fullmessage && !disconnected );
		}

		for ( int i = 0; i < toDisconnect.size(); ++i )
		{
			m_ConnectionManager->Disconnect( toDisconnect[i] );
		}
	}
	else
		MLOG_ERROR( "Attempted to receive using an uninitialized instance of Tubes", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::RequestConnection( const std::string& address, uint16_t port )
{
	if ( m_Initialized )
		m_ConnectionManager->RequestConnection( address, port );
	else
		MLOG_ERROR( "Attempted to request a connection although the Tubes instance is uninitialized", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::StartListener( uint16_t port )
{
	if ( m_Initialized )
		m_ConnectionManager->StartListener( port );
	else
		MLOG_ERROR( "Attempted to start listening on port " << port + " although the tubes instance is uninitialized", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::StopAllListeners()
{
	if ( m_Initialized )
		m_ConnectionManager->StopAllListeners();
	else
		MLOG_ERROR( "Attempted to stop all listeners although the tubes instance is uninitialized", TUBES_LOG_CATEGORY_GENERAL );
}

void Tubes::RegisterReplicator( MessageReplicator* replicator ) // TODODB: Add unregistration function
{
	m_ReplicatorReferences.emplace( replicator->GetID(), replicator ); // TODODB: Add error checking (Nullptr and duplicates)
}

ConnectionCallbackHandle Tubes::RegisterConnectionCallback( ConnectionCallbackFunction callbackFunction )
{
	return m_ConnectionManager->RegisterConnectionCallback( callbackFunction );
}

bool Tubes::UnregisterConnectionCallback( ConnectionCallbackHandle handle )
{
	return m_ConnectionManager->UnregisterConnectionCallback( handle );
}

DisconnectionCallbackHandle Tubes::RegisterDisconnectionCallback( DisconnectionCallbackFunction callbackFunction )
{
	return m_ConnectionManager->RegisterDisconnectionCallback( callbackFunction );
}

bool Tubes::UnregisterDisconnectionCallback(DisconnectionCallbackHandle handle)
{
	return m_ConnectionManager->UnregisterDisconnectionCallback( handle );
}

bool Tubes::GetHostFlag()
{
	return m_HostFlag;
}

void Tubes::SetHostFlag( bool newHostFlag )
{
	m_HostFlag = newHostFlag;
}

// ---------- LOCAL ----------

bool SendQueuedMessages( Connection& connection ) // TODODB: Handle error cases better (Don't just leave the messages in the queue)
{
	bool stillConnected = true;
	bool keepSending	= true;
	while ( !connection.unsentMessages.empty() && keepSending )
	{
		SendResult result = Communication::SendSerializedMessage( connection, connection.unsentMessages.front().first, connection.unsentMessages.front().second);
		switch ( result )
		{
			case SendResult::Sent:
			{
				connection.unsentMessages.pop();
			} break;

			case SendResult::Disconnect:
			{
				stillConnected	= false; // Signal that the socket is no longer usable and should be disconencted
				keepSending		= false;
			} break;

			case SendResult::Queued:
			case SendResult::Error:
			default:
			{
				keepSending = false;
			} break;
		}
	}

	return stillConnected;
}