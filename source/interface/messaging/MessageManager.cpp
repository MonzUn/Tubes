#include "MessageManager.h"
#include "Subscriber.h"
#include "UserMessage.h"
#include "SimulationMessage.h"
#include <MUtilityLog.h>
#include <MUtilityThreading.h>

#define MUTILITY_LOG_CATEGORY_MESSAGE_MANAGER "MessageManager"

using namespace MUtilityThreading;

MessageManager::MessageManager() {}

MessageManager::~MessageManager()
{
	LockMutexes( { &m_DeliveredUserMsgLock, &m_DeliveredSimMsgLock, &m_UserMsgQueueLock, &m_SimMsgQueueLock } );

	for ( int i = 0; i < m_DeliveredUserMessages.size(); ++i )
	{
		m_DeliveredUserMessages[i]->Destroy();
		free(m_DeliveredUserMessages[i]);
	}

	for ( int i = 0; i < m_DeliveredSimulationMessages.size(); ++i )
	{
		m_DeliveredSimulationMessages[i]->Destroy();
		free(m_DeliveredSimulationMessages[i]);
	}

	for ( int i = 0; i < m_UserMessages.size(); ++i )
	{
		m_UserMessages[i]->Destroy();
		free(m_UserMessages[i]);
	}

	for ( int i = 0; i < m_SimulationMessages.size(); ++i )
	{
		m_SimulationMessages[i]->Destroy();
		free(m_SimulationMessages[i]);
	}
	m_SimulationMessages.clear();

	UnlockMutexes( { &m_DeliveredUserMsgLock, &m_DeliveredSimMsgLock, &m_UserMsgQueueLock, &m_SimMsgQueueLock } );
}

bool MessageManager::RegisterSubscriber( Subscriber* subscriberToRegister )
{
	bool result = true;

	LockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_SimMsgQueueLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		if ( *m_Subscribers[i] == *subscriberToRegister ) // Check for duplicates
		{
			result = false;
			MLOG_WARNING("Attempted to register already registered subscriber \"" + subscriberToRegister->GetNameAsSubscriber() + "\"", MUTILITY_LOG_CATEGORY_MESSAGE_MANAGER);
			break;
		}
	}

	if ( result )
	{
		m_Subscribers.push_back( subscriberToRegister );
		CalculateInterests(); // Recalculate total interest mask
	}

	UnlockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_SimMsgQueueLock } );

	return result;
}

bool MessageManager::UnregisterSubscriber( const Subscriber* const subscriberToUnregister )
{
	bool wasUnregistered = false;

	LockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_SimMsgQueueLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i ) // Find the subscriber we want to unregister and remove it
	{
		if ( *m_Subscribers[i] == *subscriberToUnregister ) {
			m_Subscribers.erase( m_Subscribers.begin() + i );
			wasUnregistered = true;
			CalculateInterests(); // Recalculate total interest mask
			break;
		}
	}

	UnlockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_SimMsgQueueLock } );

	if ( !wasUnregistered )
		MLOG_WARNING("Attempted to unregister a non registered subscriber \"" + subscriberToUnregister->GetNameAsSubscriber() + "\"", MUTILITY_LOG_CATEGORY_MESSAGE_MANAGER);

	return wasUnregistered;
}

void MessageManager::SendImmediateUserMessage( UserMessage* message )
{
	LockMutexes( { &m_SubscriberLock, &m_DeliveredUserMsgLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		bool wasDelivered = false;
		if ( m_Subscribers[i]->GetUserInterests() & message->Type )
		{
			m_Subscribers[i]->AddUserMessage( message );
			wasDelivered = true;
		}

		if ( wasDelivered )
		{
			m_DeliveredUserMessages.push_back( message );
		}
		else
		{
			message->Destroy();
			free(message);
		}
	}

	UnlockMutexes( { &m_SubscriberLock, &m_DeliveredUserMsgLock } );
}

void MessageManager::SendImmediateSimulationMessage( SimulationMessage* message )
{
	LockMutexes( { &m_SubscriberLock, &m_DeliveredSimMsgLock } );

	bool wasDelivered = false;
	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		if ( m_Subscribers[i]->GetSimInterests() & message->Type )
		{
			m_Subscribers[i]->AddSimMessage( message );
			wasDelivered = true;
		}
	}

	if ( wasDelivered )
	{
		m_DeliveredSimulationMessages.push_back( message );
	}
	else
	{
		message->Destroy();
		free(message);
	}


	UnlockMutexes( { &m_SubscriberLock, &m_DeliveredSimMsgLock } );
}

void MessageManager::EnqueueUserMessage( UserMessage* message )
{
	LockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock } );

	if ( m_TotalUserInterests & message->Type ) // Check if any subscriber is interested in the message
	{
		m_UserMessages.push_back( message );
	}
	else
	{
		message->Destroy();
		free(message);
	}

	UnlockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock } );
}

void MessageManager::EnqueueSimulationMessage( SimulationMessage* message )
{
	LockMutexes( { &m_SubscriberLock, &m_SimMsgQueueLock } );

	if ( m_TotalSimInterests & message->Type ) // Check if any subscriber is interested in the message
	{
		m_SimulationMessages.push_back( message );
	}
	else
	{
		message->Destroy();
		free(message);
	}

	UnlockMutexes( { &m_SubscriberLock, &m_SimMsgQueueLock } );
}

void MessageManager::DeliverQueuedUserMessages()
{
	LockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_DeliveredUserMsgLock } );

	for ( int i = 0; i < m_UserMessages.size(); ++i )
	{
		bool wasDelivered = false;
		for ( int j = 0; j < m_Subscribers.size(); ++j )
		{
			if ( m_Subscribers[j]->GetUserInterests() & m_UserMessages[i]->Type )
			{
				m_Subscribers[j]->AddUserMessage( m_UserMessages[i] );
				wasDelivered = true;
			}
		}

		if( wasDelivered )
		{
			m_DeliveredUserMessages.push_back( m_UserMessages[i] );
		}
		else
		{
			m_UserMessages[i]->Destroy();
			free(m_UserMessages[i]);
		}
	}

	m_UserMessages.clear();

	UnlockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_DeliveredUserMsgLock } );
}

void MessageManager::DeliverQueuedSimulationMessages( uint64_t currentFrame )
{
	LockMutexes( { &m_SubscriberLock, &m_SimMsgQueueLock, &m_DeliveredSimMsgLock } );
	for ( int i = 0; i < m_SimulationMessages.size(); ++i )
	{
		if ( m_SimulationMessages[i]->ExecutionFrame == currentFrame )
		{
			bool wasDelivered = false;
			for ( int j = 0; j < m_Subscribers.size(); ++j )
			{
				if ( m_Subscribers[j]->GetSimInterests() & m_SimulationMessages[i]->Type )
				{
					m_Subscribers[j]->AddSimMessage( m_SimulationMessages[i] );
					wasDelivered = true;
				}
			}

			if (wasDelivered)
			{
				m_DeliveredSimulationMessages.push_back(m_SimulationMessages[i]);
			}
			else
			{
				m_SimulationMessages[i]->Destroy();
				free(m_SimulationMessages[i]);
			}

			m_SimulationMessages.erase( m_SimulationMessages.begin() + i );
			--i;
		}
	}
	UnlockMutexes( { &m_SubscriberLock, &m_SimMsgQueueLock, &m_DeliveredSimMsgLock } );
}

void MessageManager::ClearDeliveredUserMessages()
{
	LockMutexes( { &m_SubscriberLock, &m_DeliveredUserMsgLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		m_Subscribers[i]->ClearUserMessages();
	}

	for ( int i = 0; i < m_DeliveredUserMessages.size(); ++i )
	{
		m_DeliveredUserMessages[i]->Destroy();
		free(m_DeliveredUserMessages[i]);
	}

	m_DeliveredUserMessages.clear();

	UnlockMutexes( { &m_SubscriberLock, &m_DeliveredUserMsgLock } );
}

void MessageManager::ClearDeliveredSimulationMessages()
{
	LockMutexes( { &m_SubscriberLock, &m_DeliveredSimMsgLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		m_Subscribers[i]->ClearSimMessages();
	}

	for ( int i = 0; i < m_DeliveredSimulationMessages.size(); ++i )
	{
		m_DeliveredSimulationMessages[i]->Destroy();
		free(m_DeliveredSimulationMessages[i]);
	}

	m_DeliveredSimulationMessages.clear();

	UnlockMutexes( { &m_SubscriberLock, &m_DeliveredSimMsgLock } );
}

void MessageManager::DeliverUserMessage( UserMessage* message )
{
	LockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_DeliveredUserMsgLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		if ( m_Subscribers[i]->GetUserInterests() & message->Type ) // Check if the subscriber is interested in the message
			m_Subscribers[i]->AddUserMessage( message );	// Give the subscriber a reference to the message
	}

	UnlockMutexes( { &m_SubscriberLock, &m_UserMsgQueueLock, &m_DeliveredUserMsgLock } );
}

void MessageManager::DeliverSimulationMessage( SimulationMessage* message )
{
	LockMutexes( { &m_SubscriberLock, &m_SimMsgQueueLock, &m_DeliveredSimMsgLock } );

	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		if ( m_Subscribers[i]->GetSimInterests() & message->Type ) // Check if the subscriber is interested in the message
			m_Subscribers[i]->AddSimMessage( message );	// Give the subscriber a reference to the message
	}

	UnlockMutexes( { &m_SubscriberLock, &m_SimMsgQueueLock, &m_DeliveredSimMsgLock } );
}

void MessageManager::CalculateInterests() // Subscriber lock is already locked on all calling functions
{
	// Reset interests
	m_TotalSimInterests		= 0;
	m_TotalUserInterests	= 0;

	// Go through all subscribers and add their interests to the total
	for ( int i = 0; i < m_Subscribers.size(); ++i )
	{
		m_TotalUserInterests	= m_TotalUserInterests | m_Subscribers[i]->GetUserInterests();
		m_TotalSimInterests		= m_TotalSimInterests  | m_Subscribers[i]->GetSimInterests();
	}
}