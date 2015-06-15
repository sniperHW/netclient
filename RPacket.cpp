#include "RPacket.h"
#include "WPacket.h"
namespace net{
RPacket::RPacket(const WPacket &o):Packet(RPACKET,o.m_buffer){
	rpos = 4;
	pklen = m_buffer->ReadUint32(0);
	dataremain = pklen;

}

Packet *RPacket::MakeWritePacket(){
	return new WPacket(*this);
}

}