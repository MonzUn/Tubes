#include "TubesUtility.h"
#include <sstream>

std::string TubesUtility::GetErrorName( int errorCode ) // TODODB: See if the windows part can be cleaned up
{ 
#if PLATFORM == PLATFORM_WINDOWS
	wchar_t *wc = nullptr;
	FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorCode,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		reinterpret_cast<LPWSTR>( &wc ), 0, NULL );
	std::wstring ws = wc;
	std::string str;
	str.assign( ws.begin(), ws.end() );
	return str;
#else
	return strerror( errorCode );
#endif
}

std::string	TubesUtility::AddressToIPv4String( Address address )
{
	// Split address in its components
	unsigned short ip1 = ( address >> 24 );
	unsigned short ip2 = ( address >> 16 ) & 0xFF;
	unsigned short ip3 = ( address >> 8 ) & 0xFF;
	unsigned short ip4 =  address & 0xFF;

	std::stringstream stream;
	stream << ip1 << '.' << ip2 << '.' << ip3 << '.' << ip4;

	return	stream.str();
}

Address TubesUtility::IPv4StringToAddress( const std::string& addressString )
{
	// Split the adress into its parts
	uint32_t adressParts[4];
	int startSearchPos = 0;
	for ( int i = 0; i < 4; ++i )
	{
		int stopSearchPos = static_cast<int>( addressString.find( '.', startSearchPos ) );
		std::string currentAdressPart = addressString.substr( startSearchPos, stopSearchPos - startSearchPos );
		startSearchPos = stopSearchPos + 1; // +1 to not find same delimiter on next search
		adressParts[i] = static_cast<unsigned int>( std::stoul( currentAdressPart.c_str() ) );
	}

	return ( adressParts[0] << 24 ) | ( adressParts[1] << 16 ) | ( adressParts[2] << 8 ) | adressParts[3];
}