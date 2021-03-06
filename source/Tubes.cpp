#include "Interface/Tubes.h"
#include "Interface/TubesTypes.h"
#include <MUtilityPlatformDefinitions.h>
#include "TubesUtility.h"
#include "TubesMessageBase.h"
#include "TubesMessageReplicator.h"
#include "ConnectionManager.h"
#include <MUtilityLog.h>

#if PLATFORM == PLATFORM_WINDOWS
	#include <MUtilityWindowsInclude.h>
	#include <Ws2tcpip.h>
	#pragma comment( lib, "Ws2_32.lib" ) // TODODB: See if this can be done through cmake instead
#elif PLATFORM == PLATFORM_LINUX
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
#endif

#define LOG_CATEGORY_GENERAL "Tubes"

using namespace Tubes;

namespace Tubes
{
	ConnectionManager* m_ConnectionManager;

	std::unordered_map<ReplicatorID, MessageReplicator*>*	m_ReplicatorReferences;
	TubesMessageReplicator*									m_TubesMessageReplicator;

	std::vector<TubesMessage*>* m_ReceivedTubesMessages;

	bool m_Initialized = false;
}

bool Tubes::Initialize()
{ 
	if (m_Initialized)
	{
		MLOG_WARNING("Attempted to initialize an already initialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return false;
	}

	m_ReplicatorReferences = new std::unordered_map<ReplicatorID, MessageReplicator*>();
	m_ReceivedTubesMessages = new std::vector<TubesMessage*>();

#if PLATFORM == PLATFORM_WINDOWS
	WSADATA wsaData;
	m_Initialized = WSAStartup(MAKEWORD( 2, 2 ), &wsaData) == NO_ERROR; // Initialize WSA version 2.2
	if (!m_Initialized)
		LogAPIErrorMessage("Failed to initialize Tubes since WSAStartup failed", LOG_CATEGORY_GENERAL);
#else 
	m_Initialized = true;
#endif

	if (m_Initialized)
	{
		m_ConnectionManager = new ConnectionManager;
		m_TubesMessageReplicator = new TubesMessageReplicator;
		m_ReplicatorReferences->emplace(m_TubesMessageReplicator->GetID(), m_TubesMessageReplicator);

		MLOG_INFO("Tubes initialized successfully", LOG_CATEGORY_GENERAL);
	}

	return m_Initialized;
}

void Tubes::Shutdown()
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to shut down uninitialized isntance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	m_ConnectionManager->StopAllListeners();
	m_ConnectionManager->DisconnectAll();

	#if PLATFORM == PLATFORM_WINDOWS
		WSACleanup();
	#endif

	delete m_ConnectionManager;

	for (auto& IDAndReplicator : *m_ReplicatorReferences)
	{
		delete IDAndReplicator.second;
	}
	m_ReplicatorReferences->clear();
	delete m_ReplicatorReferences;

	for ( int i = 0; i < m_ReceivedTubesMessages->size(); ++i )
	{
		free((*m_ReceivedTubesMessages)[i]);
	}
	m_ReceivedTubesMessages->clear();
	delete m_ReceivedTubesMessages;

	MLOG_INFO("Tubes has been shut down", LOG_CATEGORY_GENERAL);
	m_Initialized = false;
}

void Tubes::Update()
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to update uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	m_ConnectionManager->VerifyNewConnections(*m_TubesMessageReplicator);
	m_ConnectionManager->HandleFailedConnectionAttempts();

	// Send queued messages
	std::vector<ConnectionID> toDisconnect;
	for (auto& idAndConnection : m_ConnectionManager->GetVerifiedConnections())
	{
		SendResult sendResult = idAndConnection.second->SendQueuedMessages();
		if (sendResult == SendResult::Disconnect)
		{
			toDisconnect.push_back(idAndConnection.first);
			continue;
		}
	}

	for ( int i = 0; i < toDisconnect.size(); ++i )
	{
		m_ConnectionManager->Disconnect(DisconnectionType::REMOTE_FORCEFUL, toDisconnect[i]); // TODODB: Update the disconnectionType here when we actually know if it was forceful or not
	}
}

void Tubes::SendToConnection(const Message* message, ConnectionID destinationConnectionID)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to send using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	Connection* connection = m_ConnectionManager->GetConnection(destinationConnectionID);
	if (connection != nullptr)
	{
		auto& idAndConnectionIterator = m_ReplicatorReferences->find(message->Replicator_ID);
		if (idAndConnectionIterator != m_ReplicatorReferences->end() )
		{
			SendResult result = connection->SerializeAndSendMessage(*message, *idAndConnectionIterator->second);
			switch (result)
			{
				case SendResult::Disconnect:
				{
					m_ConnectionManager->Disconnect(DisconnectionType::REMOTE_FORCEFUL, destinationConnectionID); // TODODB: Update the disconnectionType when we actually know if was forceful or not
				} break;

				case SendResult::Sent:
				case SendResult::Queued:
				case SendResult::Error:
				default:
					break;
			}
		}
		else
			MLOG_WARNING("Attempted to send message for which no replicator has been registered. Replicator ID = " << message->Replicator_ID, LOG_CATEGORY_GENERAL);
	}
	else
		MLOG_WARNING("Failed to find requested connection while sending (Requested ID = " << destinationConnectionID + " )", LOG_CATEGORY_GENERAL);
}

void Tubes::SendToAll(const Message* message, ConnectionID exception)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to send using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	auto& idAndReplicatorIterator = m_ReplicatorReferences->find(message->Replicator_ID);
	if (idAndReplicatorIterator != m_ReplicatorReferences->end())
	{
		const std::unordered_map<ConnectionID, Connection*>& connections = m_ConnectionManager->GetVerifiedConnections();

#if TUBES_DEBUG == 1
		if (exception != TUBES_INVALID_CONNECTION_ID)
		{
			bool exceptionExists = false;
			for (auto& idAndConnection : connections)
			{
				if (idAndConnection.first == exception)
					exceptionExists = true;
			}

			if (!exceptionExists)
				MLOG_WARNING("The excepted connectionID supplied to SendToAll does not exist", LOG_CATEGORY_GENERAL);
		}
#endif
		std::vector<ConnectionID> toDisconnect; // TODODB: Find a better way to disconnect connections while iterating over the connection map
		for (auto& idAndConnection : connections)
		{
			if (idAndConnection.first != exception)
			{	
				SendResult result = idAndConnection.second->SerializeAndSendMessage(*message, *idAndReplicatorIterator->second);
				switch (result)
				{
					case SendResult::Disconnect:
					{
						toDisconnect.push_back(idAndConnection.first);
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
			m_ConnectionManager->Disconnect(DisconnectionType::REMOTE_FORCEFUL, toDisconnect[i] ); // TODODB: Update the disconnectiontype when we actually know if it was forceful or not
		}
	}
	else
		MLOG_WARNING("Attempted to send message for which no replicator has been registered. Replicator ID = " << message->Replicator_ID, LOG_CATEGORY_GENERAL);
}

void Tubes::Receive(std::vector<Message*>& outMessages, std::vector<ConnectionID>* outSenderIDs)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to receive using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	std::vector<std::pair<ConnectionID, DisconnectionType>> toDisconnect; // TODODB: Find a better way to disconnect connections while iterating over the connection map
	const std::unordered_map<ConnectionID, Connection*>& connections = m_ConnectionManager->GetVerifiedConnections();
	for (auto& idAndConnection : connections)
	{
		bool disconnected = false;
		Message* message = nullptr;
		ReceiveResult result;
		do
		{
			result = idAndConnection.second->Receive(*m_ReplicatorReferences, message);
			switch (result)
			{
				case ReceiveResult::Fullmessage:
				{
					if (message->Replicator_ID == TubesMessageReplicator::TubesMessageReplicatorID)
					{
						m_ReceivedTubesMessages->push_back(reinterpret_cast<TubesMessage*>(message)); // We know that this is a tubes message
					}
					else
					{
						outMessages.push_back(message);
						if (outSenderIDs)
							outSenderIDs->push_back(idAndConnection.first);
					}
				} break;

				case ReceiveResult::GracefulDisconnect:
				case ReceiveResult::ForcefulDisconnect:
				{
					toDisconnect.push_back(std::make_pair(idAndConnection.first, result == ReceiveResult::GracefulDisconnect ? DisconnectionType::REMOTE_GRACEFUL : DisconnectionType::REMOTE_FORCEFUL));
					disconnected = true;
				} break;

				case ReceiveResult::Empty:
				case ReceiveResult::PartialMessage:
				case ReceiveResult::Error:
				default:
					break;
			}
		} while (result == ReceiveResult::Fullmessage && !disconnected);
	}

	for (int i = 0; i < toDisconnect.size(); ++i)
	{
		m_ConnectionManager->Disconnect(toDisconnect[i].second, toDisconnect[i].first);
	}
}

void Tubes::RequestConnection(const std::string& address, uint16_t port)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to request a connection although the Tubes instance is uninitialized", LOG_CATEGORY_GENERAL);
		return;
	}

	if (!Tubes::IsValidIPv4Address(address.c_str()))
	{
		m_ConnectionManager->CallConnectionCallback(ConnectionAttemptResult::FAILED_INVALID_IP);
		return;
	}

	if (port == TUBES_INVALID_PORT)
	{
		m_ConnectionManager->CallConnectionCallback(ConnectionAttemptResult::FAILED_INVALID_PORT);
		return;
	}

	m_ConnectionManager->RequestConnection(address, port);
}

bool Tubes::StartListener(uint16_t port)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to start listening on port " << port + " although the tubes instance is uninitialized", LOG_CATEGORY_GENERAL);
		return false;
	}

	return m_ConnectionManager->StartListener(port);
}
bool Tubes::StopListener(uint16_t port)
{
	if (m_Initialized)
	{
		MLOG_WARNING("Attempted to stop listener although the tubes instance is uninitialized", LOG_CATEGORY_GENERAL);
		return false;
	}

	return m_ConnectionManager->StopListener(port);
}

bool Tubes::StopAllListeners()
{
	if (!m_Initialized)
	{
		return false;
		MLOG_WARNING("Attempted to stop all listeners although the tubes instance is uninitialized", LOG_CATEGORY_GENERAL);
	}

	return m_ConnectionManager->StopAllListeners();
}

void Tubes::Disconnect(ConnectionID connectionID)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to disconnect connection using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}
	
	m_ConnectionManager->Disconnect(DisconnectionType::LOCAL, connectionID);
}

void Tubes::DisconnectAll()
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to disconnect all connections using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	m_ConnectionManager->DisconnectAll();
}

void Tubes::RegisterReplicator(MessageReplicator* replicator) // TODODB: Add unregistration function
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to register replicator using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return;
	}

	m_ReplicatorReferences->emplace(replicator->GetID(), replicator); // TODODB: Add error checking (Nullptr and duplicates)	
}

ConnectionCallbackHandle Tubes::RegisterConnectionCallback(ConnectionCallbackFunction callbackFunction)
{
	ConnectionCallbackHandle toReturn;
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to register connection callback using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return toReturn;
	}

	return m_ConnectionManager->RegisterConnectionCallback(callbackFunction);
}

bool Tubes::UnregisterConnectionCallback(ConnectionCallbackHandle handle)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to unregister connection callback using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return false;
	}

	return m_ConnectionManager->UnregisterConnectionCallback(handle);
}

DisconnectionCallbackHandle Tubes::RegisterDisconnectionCallback(DisconnectionCallbackFunction callbackFunction)
{
	DisconnectionCallbackHandle toReturn;
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to register disconnection callback using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return toReturn;
	}

	return m_ConnectionManager->RegisterDisconnectionCallback(callbackFunction);
}

bool Tubes::UnregisterDisconnectionCallback(DisconnectionCallbackHandle handle)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to unregister disconnection callback using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return false;
	}

	return m_ConnectionManager->UnregisterDisconnectionCallback(handle);
}

uint32_t Tubes::GetConnectionCount()
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to get connection count using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return 0;
	}

	return m_ConnectionManager->GetVerifiedConnctionCount();
}

ConnectionInfo Tubes::GetConnectionInfo(ConnectionID id)
{
	ConnectionInfo toReturn;
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to get connection info using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return toReturn;
	}

	if (!m_ConnectionManager->IsConnectionIDValid(id))
	{
		MLOG_WARNING("Attempted to get address of nonexistent connection (ID = " << id + " )", LOG_CATEGORY_GENERAL);
		return toReturn;
	}

	toReturn.ID = id;
	toReturn.Address = m_ConnectionManager->GetAddressOfConnection(id);
	toReturn.Port = m_ConnectionManager->GetPortOfConnection(id);		

	return toReturn;
}

std::string Tubes::GetAddressOfConnection(ConnectionID id)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to get address of connection using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return "";
	}

	if (!m_ConnectionManager->IsConnectionIDValid(id))
	{
		MLOG_WARNING("Attempted to get address of nonexistent connection (ID = " << id + " )", LOG_CATEGORY_GENERAL);
		return "";
	}

	return m_ConnectionManager->GetAddressOfConnection(id);	
}

uint16_t Tubes::GetPortOfConnection(ConnectionID id)
{
	if (!m_Initialized)
	{
		MLOG_WARNING("Attempted to get port of connection using an uninitialized instance of Tubes", LOG_CATEGORY_GENERAL);
		return TUBES_INVALID_PORT;
	}

	if (!m_ConnectionManager->IsConnectionIDValid(id))
	{
		MLOG_WARNING("Attempted to get port of nonexistent connection (ID = " << id + " )", LOG_CATEGORY_GENERAL);
		return TUBES_INVALID_PORT;
	}

	return m_ConnectionManager->GetPortOfConnection(id);
}

bool Tubes::IsValidIPv4Address(const char* ipv4String)
{
	struct sockaddr_in sa;
	int result;
#if PLATFORM == PLATFORM_WINDOWS
	result = InetPton(AF_INET, ipv4String, &(sa.sin_addr));
#else
	result = inet_pton(AF_INET, ipv4String, &(sa.sin_addr));
#endif
	return result != 0;
}