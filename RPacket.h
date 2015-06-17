#ifndef _RPACKET_H
#define _RPACKET_H

#include "Packet.h"

namespace net{

class WPacket;
class RPacket : public Packet,public StreamRPacket{
	friend class WPacket;
public:
	RPacket(ByteBuffer *buffer):Packet(RPACKET,buffer),rpos(4){
		pklen      = m_buffer->ReadUint32(0);
		dataremain = pklen;
	}

	RPacket(const RPacket &o):Packet(RPACKET,o.m_buffer),rpos(o.rpos),pklen(o.pklen){
		dataremain = pklen;
	}

	Packet *Clone(){
		return new RPacket(*this);
	}

	Packet *MakeWritePacket();

	Packet *MakeReadPacket(){
		return Clone();
	}

	RPacket(const WPacket &o);

	~RPacket(){}

	unsigned char ReadUint8(){
		if(dataremain < sizeof(unsigned char)) return 0;
		unsigned char ret = m_buffer->ReadUint8(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

	unsigned short ReadUint16(){
		if(dataremain < sizeof(unsigned short)) return 0;
		unsigned short ret = m_buffer->ReadUint16(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

	unsigned int ReadUint32(){
		if(dataremain < sizeof(unsigned int)) return 0;
		unsigned int ret = m_buffer->ReadUint32(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

	unsigned short PeekUint16(){
		if(dataremain < sizeof(unsigned short)) return 0;
		unsigned int ret = m_buffer->ReadUint16(rpos);
		return ret;		
	}

	unsigned int PeekUint32(){
		if(dataremain < sizeof(unsigned int)) return 0;
		unsigned int ret = m_buffer->ReadUint32(rpos);
		return ret;		
	}

	char ReadInt8(){
		if (dataremain < sizeof(char)) return 0;
		char ret = m_buffer->ReadInt8(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

	short ReadInt16(){
		if (dataremain < sizeof(short)) return 0;
		short ret = m_buffer->ReadInt16(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}	


	int ReadInt32(){
		if (dataremain < sizeof(unsigned int)) return 0;
		unsigned int ret = m_buffer->ReadInt32(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}


	unsigned long long ReadUint64(){
		if(dataremain < sizeof(unsigned long long)) return 0;	
		unsigned long long ret = m_buffer->ReadUint64(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

	float ReadFloat(){
		if(dataremain < sizeof(float)) return 0;
		float ret = m_buffer->ReadFloat(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

	double ReadDouble(){
		if(dataremain < sizeof(double)) return 0;
		double ret = m_buffer->ReadDouble(rpos);
		rpos += sizeof(ret);
		dataremain -= sizeof(ret);
		return ret;
	}

    void *ReadBin(size_t &len){
		len = ReadUint32();
		if(len <= 0 || len > dataremain) return NULL;
		void *ret = m_buffer->ReadBin(rpos);
		rpos += len;
		dataremain -= len;
		return ret;
	}

	const char *ReadString(){
		size_t len;
		const char *str = (const char*)ReadBin(len);
		if(str && str[len-1] == '\0')
			return str;
		return NULL;
	}

	size_t PkLen(){
		return pklen;
	}

	size_t PkTotal(){
		return PkLen() + sizeof(uint32_t);
	}	

private:

	RPacket& operator = (const RPacket &o){
		if(this != &o){
			(const_cast<ByteBuffer*>(m_buffer))->DecRef();
			m_buffer = const_cast<ByteBuffer*>(o.m_buffer)->IncRef();
			rpos = o.rpos;
		}
		return *this;
	}
	size_t      rpos;
	size_t      pklen;
	size_t      dataremain;
};

}

#endif
