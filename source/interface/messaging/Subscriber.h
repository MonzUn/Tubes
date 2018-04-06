#pragma once
#include "Message.h"
#include <mutex>
#include <vector>

struct UserMessage;
struct SimulationMessage;

class Subscriber
{
public:
	const MESSAGE_TYPE_ENUM_UNDELYING_TYPE	GetUserInterests() const;
	const MESSAGE_TYPE_ENUM_UNDELYING_TYPE	GetSimInterests() const;
	const std::string&						GetNameAsSubscriber() const;
	void									AddUserMessage( const UserMessage* message );
	void									AddSimMessage( const SimulationMessage* message );
	void									ClearUserMessages();
	void									ClearSimMessages();

	bool operator==(const Subscriber& rhs);

protected:
	// Only children may Instantiate.
	Subscriber(const std::string& name);
	virtual ~Subscriber();

	MESSAGE_TYPE_ENUM_UNDELYING_TYPE		m_SimInterests	= 0;
	MESSAGE_TYPE_ENUM_UNDELYING_TYPE		m_UserInterests = 0;
	std::vector<const UserMessage*>			m_UserMessages;
	std::vector<const SimulationMessage*>	m_SimMessages;

	std::mutex m_UserLock;
	std::mutex m_SimLock;

private:
	// No copying allowed!
	Subscriber(const Subscriber& rhs) = delete;
	Subscriber& operator=(const Subscriber& rhs) = delete;

	std::string m_Name;
};