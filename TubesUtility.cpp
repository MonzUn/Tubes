#include "TubesUtility.h"

tString TubesUtility::GetErrorName( int errorCode ) { // TODODB: See if the windows part can be cleaned up
#if PLATFORM == PLATFORM_WINDOWS
	wchar_t *wc = nullptr;
	FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorCode,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		reinterpret_cast<LPWSTR>( &wc ), 0, NULL );
	std::wstring ws = wc;
	rString str;
	str.assign( ws.begin(), ws.end() );
	return str;
#else
	return strerror( error );
#endif
}