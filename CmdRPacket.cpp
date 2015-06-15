#include "CmdRPacket.h"
#include "CmdWPacket.h"
namespace net{
CmdRPacket::CmdRPacket(const CmdWPacket &o):Packet(CMDRPACKET,o.m_buffer){
	if(m_buffer){
		m_pkhead                = o.m_pkhead;
		dataremain              = *m_pkhead.m_pktDataSize;
		rpos                    = sizeof(m_pkhead);	
	}else
		m_pkhead = {0};
}

Packet *CmdRPacket::MakeWritePacket(){
	return new CmdWPacket(*this);
}

}