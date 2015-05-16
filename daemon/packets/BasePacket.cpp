#include "BasePacket.h"
#include <cassert>
#include <ctime>

CBasePacket::CBasePacket()
: m_packetSize(PACKET_HEADER_SIZE)
, m_sourceId(0)
, m_targetId(0)
, m_commandId(0)
{

}

CBasePacket::~CBasePacket()
{

}

PacketPtr CBasePacket::Clone() const
{
	return std::make_shared<CBasePacket>(*this);
}

void CBasePacket::SetSourceId(uint32 sourceId)
{
	m_sourceId = sourceId;
}

void CBasePacket::SetTargetId(uint32 targetId)
{
	m_targetId = targetId;
}

PacketData CBasePacket::ToPacketData() const
{
	assert(m_packetSize >= PACKET_HEADER_SIZE);
	assert(m_sourceId != 0);
	assert(m_targetId != 0);

	PacketData result;
	result.resize(m_packetSize);

	//Write subpacket header
	*reinterpret_cast<uint16*>(result.data() + 0x00) = m_packetSize;
	*reinterpret_cast<uint16*>(result.data() + 0x02) = 3;			//Unknown
	*reinterpret_cast<uint32*>(result.data() + 0x04) = m_sourceId;
	*reinterpret_cast<uint32*>(result.data() + 0x08) = m_targetId;
	*reinterpret_cast<uint32*>(result.data() + 0x0C) = 0xFED2E000;	//Unknown

	//Write command header
	*reinterpret_cast<uint16*>(result.data() + 0x10) = 0x14;		//Unknown
	*reinterpret_cast<uint16*>(result.data() + 0x12) = m_commandId;
	*reinterpret_cast<uint32*>(result.data() + 0x14) = 0;
	*reinterpret_cast<uint32*>(result.data() + 0x18) = time(nullptr);
	*reinterpret_cast<uint32*>(result.data() + 0x1C) = 0;

	return result;
}
