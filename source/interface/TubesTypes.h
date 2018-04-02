#pragma once
#include <MUtilityExternal/CallbackRegister.h>
#include <string.h>

#define TUBES_PORT_ANY 0

#define TUBES_INVALID_CONNECTION_ID -1
#define TUBES_INVALID_IPv4_ADDRESS "0.0.0.0"
#define TUBES_INVALID_PORT TUBES_PORT_ANY

namespace Tubes // TODODB: Replace the callback handles so that Tubes doesn't rely on external code for this
{
	typedef int32_t	ConnectionID;

	enum class ConnectionAttemptResult : uint32_t
	{
		FAILED_INVALID_IP,
		FAILED_INVALID_PORT,
		FAILED_TIMEOUT,
		FAILED_INTERNAL_ERROR,
		SUCCESS_INCOMING,
		SUCCESS_OUTGOING,

		COUNT,
		INVALID
	};

	enum class DisconnectionType : uint32_t
	{
		LOCAL,
		REMOTE_GRACEFUL,
		REMOTE_FORCEFUL,

		COUNT,
		INVALID,
	};

	struct ConnectionAttemptResultData
	{
		ConnectionAttemptResultData() {};
		ConnectionAttemptResultData(ConnectionAttemptResult result) : Result(result) {}
		ConnectionAttemptResultData(ConnectionAttemptResult result, const std::string& address, uint16_t port, ConnectionID id ) : Result(result), Address(address), Port(port), ID(id) {}
		ConnectionAttemptResultData(ConnectionAttemptResult result, const std::string& address, uint16_t port) : Result(result), Address(address), Port(port) {}

		ConnectionAttemptResult Result	= ConnectionAttemptResult::INVALID;
		std::string Address				= TUBES_INVALID_IPv4_ADDRESS;
		uint16_t Port					= TUBES_INVALID_PORT;
		ConnectionID ID					= TUBES_INVALID_CONNECTION_ID;
	};

	struct DisconnectionData 
	{
		DisconnectionData(DisconnectionType type) : Type(type) {}
		DisconnectionData(DisconnectionType type, const std::string& address, uint16_t port, ConnectionID id) : Type(type), Address(address), Port(port), ID(id) {}

		DisconnectionType Type	= DisconnectionType::INVALID;
		std::string Address		= TUBES_INVALID_IPv4_ADDRESS;
		uint16_t Port			= TUBES_INVALID_PORT;
		ConnectionID ID			= TUBES_INVALID_CONNECTION_ID;
	};

	struct ConnectionInfo
	{
		ConnectionInfo() {}
		ConnectionInfo(ConnectionID id, const std::string& address, uint16_t port) : ID(id), Address(address), Port(Port) {}

		ConnectionID ID		= TUBES_INVALID_CONNECTION_ID;
		std::string Address = TUBES_INVALID_IPv4_ADDRESS;
		uint16_t Port		= TUBES_INVALID_PORT;
	};

	struct	ConnectionCallbackTag {};
	typedef Handle<ConnectionCallbackTag, int, -1>					ConnectionCallbackHandle;
	typedef std::function<void(const ConnectionAttemptResultData&)> ConnectionCallbackFunction;

	struct	DisconnectionCallbackTag {};
	typedef Handle<DisconnectionCallbackTag, int, -1>		DisconnectionCallbackHandle;
	typedef std::function<void(const DisconnectionData&)>	DisconnectionCallbackFunction;

	std::string ConnectionAttemptResultToString(ConnectionAttemptResult toConvert);
	std::string DisonnectionTypeToString(DisconnectionType toConvert);
}