#pragma once
#include <stdint.h>
#include <utility/PlatformDefinitions.h>

#if PLATFORM != PLATFORM_WINDOWS // winsock already defines this on windows
#define INVALID_SOCKET	~0
#define SOCKET_ERROR	-1
#endif

typedef uint16_t Port;
typedef uint64_t Socket;