#pragma once
#include <queue>
#include <mutex>
#include "Message.h" // for MESSAGE_TYPE_ENUM_UNDELYING_TYPE

class	Subscriber;
struct	UserMessage;
struct  SimulationMessage;

// TODODB: Document the shit out of this class
class MessageManager
{
public:
 	MessageManager();
	virtual ~MessageManager();

	virtual bool	RegisterSubscriber( Subscriber*	subscriberToRegister );
	virtual bool	UnregisterSubscriber( const Subscriber* const subscriberToUnregister );

	// TODODB: Add functionality for sending to specific subscribers
	virtual void	SendImmediateUserMessage		( UserMessage* message );
	virtual void	SendImmediateSimulationMessage	( SimulationMessage* message ); // This function must never race with DeliverQueuedSimMessages since that may mess up the message order and thereby the simulation.
	virtual void	EnqueueUserMessage				( UserMessage* message );
	virtual void	EnqueueSimulationMessage		( SimulationMessage* message );

protected:
	virtual void	DeliverQueuedUserMessages();
	virtual void	DeliverQueuedSimulationMessages( uint64_t currentFrame );

	virtual void	ClearDeliveredUserMessages();
	virtual void	ClearDeliveredSimulationMessages();

	std::vector<UserMessage*>		m_UserMessages;
	std::vector<SimulationMessage*>	m_SimulationMessages;
	std::vector<UserMessage*>		m_DeliveredUserMessages;
	std::vector<SimulationMessage*>	m_DeliveredSimulationMessages;

	std::mutex						m_UserMsgQueueLock;
	std::mutex						m_SimMsgQueueLock;
	std::mutex						m_DeliveredUserMsgLock;
	std::mutex						m_DeliveredSimMsgLock;

private:
	// Only one instance of each subsystem should exist so copying and comparing is illegal.
	MessageManager( const MessageManager& rhs );
	MessageManager& operator=( const MessageManager& rhs );

	void DeliverUserMessage			( UserMessage* message );
	void DeliverSimulationMessage	( SimulationMessage* message );
	void CalculateInterests			();

	MESSAGE_TYPE_ENUM_UNDELYING_TYPE		m_TotalSimInterests	 = 0;
	MESSAGE_TYPE_ENUM_UNDELYING_TYPE		m_TotalUserInterests = 0;

	std::vector<Subscriber*>				m_Subscribers;

	std::mutex								m_SubscriberLock;
};