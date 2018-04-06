#include "Subscriber.h"

Subscriber::Subscriber(const std::string& name)
{
	m_Name = name;
}

Subscriber::~Subscriber() { }

bool Subscriber::operator==(const Subscriber& rhs)
{
	return this->m_Name == rhs.m_Name;
}

const MESSAGE_TYPE_ENUM_UNDELYING_TYPE Subscriber::GetUserInterests() const
{
	return m_UserInterests;
}

const MESSAGE_TYPE_ENUM_UNDELYING_TYPE Subscriber::GetSimInterests() const
{
	return m_SimInterests;
}

const std::string& Subscriber::GetNameAsSubscriber() const
{
	return m_Name;
}

void Subscriber::AddUserMessage(const UserMessage* message)
{
	m_UserLock.lock();
	m_UserMessages.push_back(message);
	m_UserLock.unlock();
}

void Subscriber::AddSimMessage(const SimulationMessage* message)
{
	m_SimLock.lock();
	m_SimMessages.push_back(message);
	m_SimLock.unlock();
}

void Subscriber::ClearUserMessages()
{
	m_UserLock.lock();
	m_UserMessages.clear( );
	m_UserLock.unlock();
}

void Subscriber::ClearSimMessages()
{
	m_SimLock.lock();
	m_SimMessages.clear( );
	m_SimLock.unlock();
}