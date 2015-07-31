#pragma once
#include <stdint.h>
#include <thread>
#include <atomic>
#include <utility/PlatformDefinitions.h>
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