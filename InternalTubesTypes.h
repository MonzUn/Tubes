#pragma once
#include <stdint.h>
#include <thread>
#include <atomic>
#include <utility/PlatformDefinitions.h>
#include <utility/Byte.h>
#include <utility/DataSizes.h>
#include <memory/Alloc.h>
#include <messaging/MessagingTypes.h>
#include "TubesErrors.h"

#if PLATFORM != PLATFORM_WINDOWS // winsock already defines this on windows
#define INVALID_SOCKET	~0
#define SOCKET_ERROR	-1
#endif

typedef uint32_t	Address;
typedef uint16_t	Port;
typedef int64_t		Socket;
typedef uint32_t	ConnectionID;

#if PLATFORM == PLATFORM_WINDOWS
typedef int socklen_t;
#endif

enum ConnectionState {
	NEW_OUT,
	NEW_IN,
};

struct Listener {
	Listener() {
		Thread = nullptr;
		ListeningSocket = INVALID_SOCKET;
		ShouldTerminate = pNew( std::atomic_bool, false );
	}

	~Listener() {
		pDelete( Thread );
		pDelete( ShouldTerminate );
	}

	Socket				ListeningSocket;
	std::thread*		Thread;
	std::atomic_bool*	ShouldTerminate;
};

struct ReceiveBuffer {
	ReceiveBuffer();

	int16_t		ExpectedHeaderBytes;	// How many more bytes of header we expect for the current packet
	MessageSize ExpectedPayloadBytes;	// How many more bytes of payload we expect for the current packet
	Byte*		PayloadData;			// Dynamic buffer for packet payload
	Byte*		Walker;					// Pointer to where the next recv should write to
};