#ifndef _RAWBINPACKET_H
#define _RAWBINPACKET_H


#include "Packet.h"


namespace net{

class RawBinPacket : public Packet{

public:
	RawBinPacket(const char *data,size_t len):Packet(RAWBINARY,new ByteBuffer(len)),m_size(len)
	{
		m_buffer->WriteBin(0,(void*)data,len);
	}

	RawBinPacket(const RawBinPacket &o):Packet(RAWBINARY,o.m_buffer),m_size(o.m_size)
	{}

	RawBinPacket& operator = (const RawBinPacket &o){
		if(this != &o){
			if(m_buffer){
				m_buffer->DecRef();
				m_buffer = NULL;
				m_size   = 0;
			}
			if(o.m_buffer){
				m_buffer = o.m_buffer->IncRef();
				m_size   = o.m_size;
			}
		}
		return *this;
	}

	const char *ReadBin(size_t &len){
		len = m_size;
		return (const char *)(&m_buffer->Buf()[0]);
	}

	Packet *Clone(){
		return new RawBinPacket(*this);
	}

	Packet *MakeWritePacket(){
		return NULL;
	}

	Packet *MakeReadPacket(){
		return Clone();
	}	

	~RawBinPacket(){}

	size_t PkLen(){
		return m_size;
	}

	size_t PkTotal(){
		return PkLen();
	}		

private:
	size_t m_size;

};

}

#endif