#pragma once
#include <MUtilityExternal/CallbackRegister.h>

#define INVALID_CONNECTION_ID -1
typedef int32_t	ConnectionID;

struct	ConnectionCallbackTag {};
typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
typedef std::function<void( int32_t )>			ConnectionCallbackFunction;