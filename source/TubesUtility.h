#pragma once
#include "InternalTubesTypes.h"
#include <MUtilityPlatformDefinitions.h>
#include <MUtilityWindowsInclude.h>

#if PLATFORM == PLATFORM_WINDOWS
#define GET_NETWORK_ERROR WSAGetLastError()
#elif PLATFORM == PLATFORM_LINUX
#define GET_NETWORK_ERROR errno
#endif

// Always output errors through this define or string constructors may overwrite errno
#define LogInfoMessage( outputMessage ) {}		//{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::INFO_MSG ); } // TODODB: Rewrite how logging is handled so that Tubes doesn't write output directly (Some buffer system is needed)
#define LogDebugMessage( outputMessage ) {}	//{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::DEBUG_MSG ); }
#define LogWarningMessage( outputMessage ) {}	//{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::WARNING_MSG ); }
#define LogErrorMessage( outputMessage ) {}	//{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( outputMessage, TubesUtility::LOGGER_NAME, LogSeverity::ERROR_MSG ); }
#define LogAPIErrorMessage( outputMessage ) {}	//{ int __errorCode = GET_NETWORK_ERROR; Logger::Log( tString( outputMessage " - Error: " ) + TubesUtility::GetErrorName( __errorCode ), TubesUtility::LOGGER_NAME , LogSeverity::ERROR_MSG ); } // TODODB: Go through the code and see where this output type is applicable

#define LOCALHOST_IP "127.0.0.1"
#define TUBES_DEBUG 1
#define NOT_EXPECTING_PAYLOAD -1

namespace TubesUtility
{
	const std::string	LOGGER_NAME				= "Tubes";
	const int			MAX_MESSAGE_SIZE		= 512;				// This low message size cap will lower the risks of packet loss and/or corruption
	const int			MAX_LISTENING_BACKLOG	= 16;				// How many incoming connections that can be connecting at the same time

	std::string			GetErrorName( int errorCode );
	std::string			AddressToIPv4String( Address address );
	Address				IPv4StringToAddress( const std::string& addressString );
}