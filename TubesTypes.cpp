#include "TubesTypes.h"
#include "TubesUtility.h"

ReceiveBuffer::ReceiveBuffer() {
	ExpectedHeaderBytes		= DataSizes::INT_64_SIZE;
	ExpectedPayloadBytes	= NOT_EXPECTING_PAYLOAD;
	PayloadData				= nullptr;
	Walker					= reinterpret_cast<Byte*>( &ExpectedPayloadBytes );
}