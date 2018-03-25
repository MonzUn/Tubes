#pragma once
#include <MUtilityExternal/CallbackRegister.h>


namespace Tubes // TODODB: Replace the callback handles so that Tubes dopesn't rely on external code for this
#define TUBES_INVALID_CONNECTION_ID -1
{
	typedef int32_t	ConnectionID;

	struct	ConnectionCallbackTag {};
	typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>		ConnectionCallbackFunction;

	struct	DisconnectionCallbackTag {};
	typedef Handle<DisconnectionCallbackTag, int, -1>	DisconnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>			DisconnectionCallbackFunction;
}