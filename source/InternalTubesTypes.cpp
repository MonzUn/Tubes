#include "InternalTubesTypes.h"
#include "interface/messaging/MessagingTypes.h"
#include "TubesUtility.h"

ReceiveBuffer::ReceiveBuffer()
{
	Reset();
}

ReceiveBuffer::~ReceiveBuffer()
{
	if ( PayloadData != nullptr )
		free( PayloadData );
}

void ReceiveBuffer::Reset()
{
	ExpectedHeaderBytes		= sizeof(MessageSize);
	ExpectedPayloadBytes	= NOT_EXPECTING_PAYLOAD;
	PayloadData				= nullptr;
	Walker					= reinterpret_cast<MUtility::Byte*>(&ExpectedPayloadBytes);
}