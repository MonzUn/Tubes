#pragma once
#include "Message.h"

struct SimulationMessage : public Message
{
	SimulationMessage(MESSAGE_TYPE_ENUM_UNDELYING_TYPE type, ReplicatorID replicatorID, uint64_t executionFrame) : Message(type, replicatorID) { ExecutionFrame = executionFrame; };

	uint64_t ExecutionFrame;
};