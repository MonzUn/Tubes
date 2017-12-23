#pragma once
#include "Message.h"

struct UserMessage : public Message
{
	UserMessage( MESSAGE_TYPE_ENUM_UNDELYING_TYPE type, ReplicatorID replicatorID ) : Message( type, replicatorID ) {}
};