#pragma once
#include <MUtilityExternal/CallbackRegister.h>

#define INVALID_CONNECTION_ID -1

namespace Tubes
{
	typedef int32_t	ConnectionID;

	struct	ConnectionCallbackTag {};
	typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>		ConnectionCallbackFunction;

	struct	DisconnectionCallbackTag {};
	typedef Handle<DisconnectionCallbackTag, int, -1>	DisconnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>			DisconnectionCallbackFunction;
}