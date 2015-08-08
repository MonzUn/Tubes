#pragma once
#include <messaging/Message.h>

struct TubesMessage : Message {
	TubesMessage( MESSAGE_TYPE_ENUM_UNDELYING_TYPE type ) : Message( type, 0 ) {} // TODODB: Use replicator ID here instead
};