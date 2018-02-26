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
	MUtility::Serialization::WriteInt16( value, m_WritingWalker );
}

void MessageReplicator::WriteInt32( int32_t value )
{
	MUtility::Serialization::WriteInt32( value, m_WritingWalker );
}

void MessageReplicator::WriteInt64( int64_t value )
{
	MUtility::Serialization::WriteInt64( value, m_WritingWalker );
}

void MessageReplicator::WriteUint16( uint16_t value )
{
	MUtility::Serialization::WriteUint16( value, m_WritingWalker );
}

void MessageReplicator::WriteUint32( uint32_t value )
{
	MUtility::Serialization::WriteUint32( value, m_WritingWalker );
}

void MessageReplicator::WriteUint64( uint64_t value )
{
	MUtility::Serialization::WriteUint64( value, m_WritingWalker );
}

void MessageReplicator::WriteFloat( float value )
{
	MUtility::Serialization::WriteFloat( value, m_WritingWalker );
}

void MessageReplicator::WriteDouble( double value )
{
	MUtility::Serialization::WriteDouble( value, m_WritingWalker );
}

void MessageReplicator::WriteBool( bool value )
{
	MUtility::Serialization::WriteBool( value, m_WritingWalker );
}

void MessageReplicator::WriteString( const std::string& value )
{
	MUtility::Serialization::WriteString( value, m_WritingWalker );
}
	 
void MessageReplicator::ReadInt16( int16_t&	value )
{
	MUtility::Serialization::ReadInt16( value, m_ReadingWalker );
}

void MessageReplicator::ReadInt32( int32_t& value )
{
	MUtility::Serialization::ReadInt32( value, m_ReadingWalker );
}

void MessageReplicator::ReadInt64( int64_t&	value )
{
	MUtility::Serialization::ReadInt64( value, m_ReadingWalker );
}

void MessageReplicator::ReadUint16( uint16_t& value )
{
	MUtility::Serialization::ReadUInt16( value, m_ReadingWalker );
}

void MessageReplicator::ReadUint32( uint32_t& value )
{
	MUtility::Serialization::ReadUint32( value, m_ReadingWalker );
}

void MessageReplicator::ReadUint64( uint64_t& value )
{
	MUtility::Serialization::ReadUint64( value, m_ReadingWalker );
}

void MessageReplicator::ReadFloat( float& value )
{
	MUtility::Serialization::ReadFloat( value, m_ReadingWalker );
}

void MessageReplicator::ReadDouble( double& value )
{
	MUtility::Serialization::ReadDouble( value, m_ReadingWalker );
}

void MessageReplicator::ReadBool( bool&	value )
{
	MUtility::Serialization::ReadBool( value, m_ReadingWalker );
}

void MessageReplicator::ReadString( std::string& value )
{
	MUtility::Serialization::ReadString( value, m_ReadingWalker );
}
