#pragma once
#include "TubesMessageBase.h"

namespace TubesMessages
{
	enum MessageType : MESSAGE_TYPE_ENUM_UNDELYING_TYPE
	{
		CONNECTION_ID,
	};
}

struct ConnectionIDMessage : TubesMessage
{
	ConnectionIDMessage( ConnectionID id ) : TubesMessage( TubesMessages::CONNECTION_ID ) { ID = id; }

	ConnectionID ID;
};