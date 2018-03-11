#pragma once
#include <MUtilityExternal/CallbackRegister.h>

#define INVALID_TUBES_CONNECTION_ID -1

namespace Tubes // TODODB: Replace the callback handles so that Tubes dopesn't rely on external code for this
{
	typedef int32_t	ConnectionID;

	struct	ConnectionCallbackTag {};
	typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>		ConnectionCallbackFunction;

	struct	DisconnectionCallbackTag {};
	typedef Handle<DisconnectionCallbackTag, int, -1>	DisconnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>			DisconnectionCallbackFunction;
}