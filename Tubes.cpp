#include "Tubes.h"
#include <utility/PlatformDefinitions.h>
#include "TubesUtility.h"

#if PLATFORM == PLATFORM_WINDOWS
	#include <utility/RetardedWindowsIncludes.h>
	#pragma comment( lib, "Ws2_32.lib" ) // TODODB: See if this can be done through cmake instead
#elif PLATFORM == PLATFORM_LINUX
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
#endif

bool Tubes::Initialize() {
#if PLATFORM == PLATFORM_WINDOWS
	WSADATA wsaData;
	m_Initialized = WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == NO_ERROR; // Initialize WSA version 2.2
	if ( !m_Initialized ) {
			LogErrorMessage( "Failed to initialize Tubes since WSAStartup failed" );	
	}
#else 
	m_Initialized = true;
#endif

	if ( m_Initialized ) {
		LogInfoMessage( "Tubes was successfully initialized" );
	}

	return m_Initialized;
}

void Tubes::Shutdown() {
	if ( m_Initialized ) {
		m_ConnectionManager.StopAllListeners();

		#if PLATFORM == PLATFORM_WINDOWS
			WSACleanup();
		#endif
		LogInfoMessage( "Tubes has been shut down" );
	} else {
		LogWarningMessage( "Attempted to shut down uninitialized isntance of Tubes" );
	}

	m_Initialized = false;
}

void Tubes::Update() {
	if ( m_Initialized ) {

	} else {
		LogWarningMessage( "Attempted to update uninitialized instance of Tubes" );
	}
}

void Tubes::RequestConnection( const tString& address, uint16_t port ) {
	if ( m_Initialized ) {
		m_ConnectionManager.RequestConnection( address, port );
	} else {
		LogWarningMessage( "Attempted to request a connection although the Tubes instance is uninitialized" );
	}
}

void Tubes::StartListener( uint16_t port ) {
	if ( m_Initialized ) {
		m_ConnectionManager.StartListener( port );
	} else {
		LogWarningMessage( "Attempted to start listening on port " + rToString( port ) + " although the tubes instance is uninitialized" );
	}
}

void Tubes::StopAllListeners() {
	if ( m_Initialized ) {
		m_ConnectionManager.StopAllListeners();
	} else {
		LogWarningMessage( "Attempted to stop all listeners although the tubes instance is uninitialized" );
	}
}

bool Tubes::GetHostFlag() const {
	return m_HostFlag;
}

void Tubes::SetHostFlag( bool newHostFlag ) {
	m_HostFlag = newHostFlag;
}