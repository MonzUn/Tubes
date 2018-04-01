#pragma once
#include <MUtilityExternal/CallbackRegister.h>

#define TUBES_PORT_ANY 0

#define TUBES_INVALID_CONNECTION_ID -1
#define TUBES_INVALID_IPv4_ADDRESS "0.0.0.0"
#define TUBES_INVALID_PORT TUBES_PORT_ANY

namespace Tubes // TODODB: Replace the callback handles so that Tubes doesn't rely on external code for this
{
	typedef int32_t	ConnectionID;

	enum class ConnectionAttemptResult
	{
		FAILED_INVALID_IP,
		FAILED_INVALID_PORT,
		FAILED_TIMEOUT,
		FAILED_INTERNAL_ERROR,
		SUCCESS_INCOMING,
		SUCCESS_OUTGOING,

		INVALID
	};

	struct ConnectionAttemptResultData
	{
		ConnectionAttemptResultData() {};
		ConnectionAttemptResultData(ConnectionAttemptResult result, const std::string& address, uint16_t port, ConnectionID id ) : Result(result), Address(address), Port(port), ID(id) {}
		ConnectionAttemptResultData(ConnectionAttemptResult result, const std::string& address, uint16_t port) : Result(result), Address(address), Port(port) {}

		ConnectionAttemptResult Result	= ConnectionAttemptResult::INVALID;
		std::string Address				= TUBES_INVALID_IPv4_ADDRESS;
		uint16_t Port					= TUBES_INVALID_PORT;
		ConnectionID ID					= TUBES_INVALID_CONNECTION_ID;
	};

	struct	ConnectionCallbackTag {};
	typedef Handle<ConnectionCallbackTag, int, -1>	ConnectionCallbackHandle;
	typedef std::function<void(ConnectionAttemptResultData)> ConnectionCallbackFunction;

	struct	DisconnectionCallbackTag {};
	typedef Handle<DisconnectionCallbackTag, int, -1>	DisconnectionCallbackHandle;
	typedef std::function<void(ConnectionID)>			DisconnectionCallbackFunction; // TODODB: Return a struct similar to ConnectionAttemptResult here to better explain who and why
}