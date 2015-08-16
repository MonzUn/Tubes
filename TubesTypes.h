#pragma once
#include <utility/CallbackRegister.h>

#define INVALID_CONNECTION_ID -1
typedef int32_t	ConnectionID;

struct	ConnectionCallbackTag {};
typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
typedef std::function<void( uint32_t )>			ConnectionCallbackFunction;