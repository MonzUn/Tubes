#pragma once
#include "InternalTubesTypes.h"
#include <MUtilityLog.h>
#include <MUtilityPlatformDefinitions.h>
#include <MUtilityWindowsInclude.h>

#if PLATFORM == PLATFORM_WINDOWS
#define GET_NETWORK_ERROR WSAGetLastError()
#elif PLATFORM == PLATFORM_LINUX
#define GET_NETWORK_ERROR errno
#endif

#define NOT_EXPECTING_PAYLOAD -1

// Always output API related errors through this define or string constructors may overwrite errno
#define LogAPIErrorMessage(outputMessage, logCategory) { int __errorCode = GET_NETWORK_ERROR; MLOG_ERROR(outputMessage << " - Error (" << __errorCode << ") " << TubesUtility::GetErrorName(__errorCode), logCategory); }

#define LOCALHOST_IP "127.0.0.1"
#define TUBES_DEBUG 1

namespace TubesUtility
{
	const int			MAX_MESSAGE_SIZE = 512; // This low message size cap will lower the risks of packet loss and/or corruption

	std::string			GetErrorName( int errorCode );
	std::string			AddressToIPv4String( Address address );
	Address				IPv4StringToAddress( const std::string& addressString );

	void				ShutdownAndCloseSocket( Socket& socket );
	void				CloseSocket( Socket& socket );
}