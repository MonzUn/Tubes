#pragma once
#include "MessagingTypes.h"
#include "Message.h"
#include <MUtilityByte.h>
#include <string>

class MessageReplicator
{
public:
	MessageReplicator( ReplicatorID id );

	virtual MUtility::Byte*	SerializeMessage( const Message* message, MessageSize* outMessageSize = nullptr, MUtility::Byte* optionalWritingBuffer = nullptr ) = 0;
	virtual Message*		DeserializeMessage( const MUtility::Byte* const buffer ) = 0; // TODODB: Return how many bytes were read instead // TODODB: Take optionalwritingbuffer as parameter
	virtual MessageSize		CalculateMessageSize( const Message& message ) const = 0;
	
	ReplicatorID		GetID() const;
	
	void				WriteInt16(					int16_t			value );
	void				WriteInt32(					int32_t			value );
	void				WriteInt64(					int64_t			value );
	void				WriteUint16(				uint16_t		value );
	void				WriteUint32(				uint32_t		value );
	void				WriteUint64(				uint64_t		value );
	void				WriteFloat(					float			value );
	void				WriteDouble(				double			value );
	void				WriteBool(					bool			value );
	void				WriteString(		const	std::string&	value );
	
	void				ReadInt16(					int16_t&		value  );
	void				ReadInt32(					int32_t&		value  );
	void				ReadInt64(					int64_t&		value  );
	void				ReadUint16(					uint16_t&		value  );
	void				ReadUint32(					uint32_t&		value  );
	void				ReadUint64(					uint64_t&		value  );
	void				ReadFloat(					float&			value  );
	void				ReadDouble(					double&			value  );
	void				ReadBool(					bool&			value  );
	void				ReadString(					std::string&	value  );

protected:
	MUtility::Byte*			m_WritingWalker		= nullptr;
	const MUtility::Byte*	m_ReadingWalker		= nullptr;

private:
	ReplicatorID m_ID;
};