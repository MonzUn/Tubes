#pragma once
#include <memory/Alloc.h>
#include <utility/Logger.h>
#include <utility/PlatformDefinitions.h>
#if PLATFORM == PLATFORM_WINDOWS
#include <utility/RetardedWindowsIncludes.h>
#elif PLATFORM == PLATFORM_LINUX
#include <string.h> // Needed for strerror
#endif

#if PLATFORM == PLATFORM_WINDOWS
#define GET_NETWORK_ERROR WSAGetLastError()
#elif PLATFORM == PLATFORM_LINUX
#define GET_NETWORK_ERROR errno
#endif

#if PLATFORM != PLATFORM_WINDOWS // winsock already defines this on windows
#define INVALID_SOCKET	~0
#define SOCKET_ERROR	-1
#endif

// Always output errors through this define or string constructors may overwrite errno
#define LogInfoMessage( outputMessage )		{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::INFO_MSG ); }
#define LogDebugMessage( outputMessage )	{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::DEBUG_MSG ); }
#define LogWarningMessage( outputMessage )	{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::WARNING_MSG ); }
#define LogErrorMessage( outputMessage )	{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( tString( outputMessage " - Error: " ) + TubesUtility::GetErrorName( __errorCode ), TubesUtility::LOGGER_NAME , LogSeverity::ERROR_MSG ); }

#define LOCALHOST_IP	"127.0.0.1"
#define NETWORK_DEBUG 1

// Define error codes in a cross platform manner
#if PLATFORM == PLATFORM_WINDOWS
#define TUBES_EWOULDBLOCK			WSAEWOULDBLOCK
#define TUBES_ECONNECTIONABORTED	WSAECONNABORTED
#define TUBES_ECONNRESET			WSAECONNRESET
#define TUBES_EINTR					WSAEINTR
#elif PLATFORM == PLATFORM_LINUX
#define TUBES_EWOULDBLOCK			EWOULDBLOCK
#define TUBES_ECONNECTIONABORTED	ECONNABORTED
#define TUBES_ECONNRESET			ECONNRESET
#define TUBES_EINTR					EINTR
#endif

namespace TubesUtility {
	typedef unsigned long long Socket;
	const pString	LOGGER_NAME				= "Tubes";
	const int		MAX_MESSAGE_SIZE		= 512;				// This is low message size cap will lower the risks of packet loss and/or corruption
	const int		MAX_LISTENING_BACKLOG	= 16;				// How many incoming connections that can be connecting at the same time

	tString			GetErrorName( int errorCode );
}