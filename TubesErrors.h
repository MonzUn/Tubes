#pragma once
#include <utility/PlatformDefinitions.h>
#if PLATFORM == PLATFORM_WINDOWS
#include <utility/RetardedWindowsIncludes.h>
#endif

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