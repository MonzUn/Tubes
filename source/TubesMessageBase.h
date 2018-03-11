#pragma once
#include "Interface/messaging/Message.h"
#include "TubesMessageReplicator.h"

struct TubesMessage : Message
{
	TubesMessage( MESSAGE_TYPE_ENUM_UNDELYING_TYPE type ) : Message( type, TubesMessageReplicator::TubesMessageReplicatorID ) {}
};