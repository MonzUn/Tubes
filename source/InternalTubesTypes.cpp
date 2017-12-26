#include "InternalTubesTypes.h"
#include "interface/messaging/MessagingTypes.h"
#include "TubesUtility.h"

ReceiveBuffer::ReceiveBuffer()
{
	ExpectedHeaderBytes		= sizeof( MessageSize );
	ExpectedPayloadBytes	= NOT_EXPECTING_PAYLOAD;
	PayloadData				= nullptr;
	Walker					= reinterpret_cast<Byte*>( &ExpectedPayloadBytes );
}

ReceiveBuffer::~ReceiveBuffer()
{
	if ( PayloadData != nullptr )
		free( PayloadData );
}