#include "Interface/TubesTypes.h"
#include <MUtilityMacros.h>
#include <type_traits>

using namespace Tubes;

constexpr int32_t EnumStringMaxLength = 30;

char ConnectionAttemptResultStrings[static_cast<std::underlying_type<ConnectionAttemptResult>::type>(ConnectionAttemptResult::COUNT)][EnumStringMaxLength] =
{
	MUTILITY_STRINGIFY(FAILED_INVALID_IP),
	MUTILITY_STRINGIFY(FAILED_INVALID_PORT),
	MUTILITY_STRINGIFY(FAILED_TIMEOUT),
	MUTILITY_STRINGIFY(FAILED_INTERNAL_ERROR),
	MUTILITY_STRINGIFY(SUCCESS_INCOMING),
	MUTILITY_STRINGIFY(SUCCESS_OUTGOING)
};

char DisconnectionTypeStrings[static_cast<std::underlying_type<DisconnectionType>::type>(DisconnectionType::COUNT)][EnumStringMaxLength] =
{
	MUTILITY_STRINGIFY(LOCAL),
	MUTILITY_STRINGIFY(REMOTE_GRACEFUL),
	MUTILITY_STRINGIFY(REMOTE_FORCEFUL),
};

std::string Tubes::ConnectionAttemptResultToString(ConnectionAttemptResult toConvert)
{
	if(toConvert >= ConnectionAttemptResult::COUNT)
		return "";

	return ConnectionAttemptResultStrings[static_cast<std::underlying_type<ConnectionAttemptResult>::type>(toConvert)];
}

std::string Tubes::DisonnectionTypeToString(DisconnectionType toConvert)
{
	if (toConvert >= DisconnectionType::COUNT)
		return "";

	return DisconnectionTypeStrings[static_cast<std::underlying_type<DisconnectionType>::type>(toConvert)];
}