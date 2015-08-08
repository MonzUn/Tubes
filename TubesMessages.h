#pragma once
#include "TubesMessageBase.h"
#include "TubesTypes.h"

namespace Messages {
	enum MessageType : MESSAGE_TYPE_ENUM_UNDELYING_TYPE {
		CONNECTION_ID,
	};
}

struct ConnectionIDMessage : TubesMessage {
	ConnectionIDMessage( ConnectionID id ) : TubesMessage( Messages::CONNECTION_ID ) { ID = id; }

	ConnectionID ID;
};