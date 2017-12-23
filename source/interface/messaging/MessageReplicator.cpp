#include "MessageReplicator.h"
#include <MUtilitySerialization.h>

MessageReplicator::MessageReplicator( ReplicatorID id )
{
	m_ID = id;
}

ReplicatorID MessageReplicator::GetID() const
{
	return m_ID;
}

void MessageReplicator::WriteInt16( int16_t value )
{
	MUtilitySerialization::WriteInt16( value, m_WritingWalker );
}

void MessageReplicator::WriteInt32( int32_t value )
{
	MUtilitySerialization::WriteInt32( value, m_WritingWalker );
}

void MessageReplicator::WriteInt64( int64_t value )
{
	MUtilitySerialization::WriteInt64( value, m_WritingWalker );
}

void MessageReplicator::WriteUint16( uint16_t value )
{
	MUtilitySerialization::WriteUint16( value, m_WritingWalker );
}

void MessageReplicator::WriteUint32( uint32_t value )
{
	MUtilitySerialization::WriteUint32( value, m_WritingWalker );
}

void MessageReplicator::WriteUint64( uint64_t value )
{
	MUtilitySerialization::WriteUint64( value, m_WritingWalker );
}

void MessageReplicator::WriteFloat( float value )
{
	MUtilitySerialization::WriteFloat( value, m_WritingWalker );
}

void MessageReplicator::WriteDouble( double value )
{
	MUtilitySerialization::WriteDouble( value, m_WritingWalker );
}

void MessageReplicator::WriteBool( bool value )
{
	MUtilitySerialization::WriteBool( value, m_WritingWalker );
}

void MessageReplicator::WriteString( const std::string& value )
{
	MUtilitySerialization::WriteString( value, m_WritingWalker );
}
	 
void MessageReplicator::ReadInt16( int16_t&	value )
{
	MUtilitySerialization::ReadInt16( value, m_ReadingWalker );
}

void MessageReplicator::ReadInt32( int32_t& value )
{
	MUtilitySerialization::ReadInt32( value, m_ReadingWalker );
}

void MessageReplicator::ReadInt64( int64_t&	value )
{
	MUtilitySerialization::ReadInt64( value, m_ReadingWalker );
}

void MessageReplicator::ReadUint16( uint16_t& value )
{
	MUtilitySerialization::ReadUInt16( value, m_ReadingWalker );
}

void MessageReplicator::ReadUint32( uint32_t& value )
{
	MUtilitySerialization::ReadUint32( value, m_ReadingWalker );
}

void MessageReplicator::ReadUint64( uint64_t& value )
{
	MUtilitySerialization::ReadUint64( value, m_ReadingWalker );
}

void MessageReplicator::ReadFloat( float& value )
{
	MUtilitySerialization::ReadFloat( value, m_ReadingWalker );
}

void MessageReplicator::ReadDouble( double& value )
{
	MUtilitySerialization::ReadDouble( value, m_ReadingWalker );
}

void MessageReplicator::ReadBool( bool&	value )
{
	MUtilitySerialization::ReadBool( value, m_ReadingWalker );
}

void MessageReplicator::ReadString( std::string& value )
{
	MUtilitySerialization::ReadString( value, m_ReadingWalker );
}
