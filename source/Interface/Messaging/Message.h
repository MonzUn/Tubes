#pragma once
#include <stdint.h>
#include "MessagingTypes.h"

#define MESSAGE_TYPE_ENUM_UNDELYING_TYPE uint64_t
#define MESSAGE_TYPE_HALF_BIT_SIZE ( sizeof( MESSAGE_TYPE_ENUM_UNDELYING_TYPE ) * 4 ) // *4 since we want the bit count instead of byte count and we want half the size (8/2)
#define MESSAGE_TYPE_BITFLAG_MIDDLE ( 1ULL << MESSAGE_TYPE_HALF_BIT_SIZE )

struct Message
{
public:
	Message( MESSAGE_TYPE_ENUM_UNDELYING_TYPE type, ReplicatorID replicatorID ) : Type( type ), Replicator_ID( replicatorID ) {}

	virtual void Destroy() {};
	
	MESSAGE_TYPE_ENUM_UNDELYING_TYPE	Type			= 0;
	ReplicatorID						Replicator_ID	= INVALID_REPLICATOR_ID; // TODODB: See if the _ in Replicator_ID can be removed somehow

protected:
	~Message() {}; // Messages are always allocated using malloc and thus the destructor should never be called. Use Destroy() instead
};