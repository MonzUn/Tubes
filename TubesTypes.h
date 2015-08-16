#pragma once
#include <utility/CallbackRegister.h>

typedef uint32_t	ConnectionID;

struct	ConnectionCallbackTag {};
typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
typedef std::function<void( uint32_t )>			ConnectionCallbackFunction;