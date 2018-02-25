#pragma once
#include <stdint.h>
#include <thread>
#include <atomic>
#include <MUtilityPlatformDefinitions.h>
#include <MUtilityByte.h>
#include <MUtilityDataSizes.h>
#include "interface/messaging/MessagingTypes.h"
#include "TubesErrors.h"

#if PLATFORM != PLATFORM_WINDOWS // winsock already defines this on windows
#define INVALID_SOCKET	~0
#define SOCKET_ERROR	-1
#endif

typedef uint32_t	Address;
typedef uint16_t	Port;
typedef int64_t		Socket;

#if PLATFORM == PLATFORM_WINDOWS
typedef int socklen_t;
#endif

enum class ConnectionState
{
	NewOutgoing,
	NewIncoming,
};

struct ReceiveBuffer
{
	ReceiveBuffer();
	~ReceiveBuffer();

	int16_t				ExpectedHeaderBytes;	// How many more bytes of header we expect for the current packet
	MessageSize			ExpectedPayloadBytes;	// How many more bytes of payload we expect for the current packet
	MUtility::Byte*		PayloadData;			// Dynamic buffer for packet payload
	MUtility::Byte*		Walker;					// Pointer to where the next recv should write to
};