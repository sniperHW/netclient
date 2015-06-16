#ifndef _CMDRPACKET_H
#define _CMDRPACKET_H

#include "Packet.h"

namespace net{

//特定游戏的包结构

class CmdWPacket;
class CmdRPacket : public Packet,public StreamRPacket{
	friend class CmdWPacket;
public:
	CmdRPacket(ByteBuffer *buffer):Packet(CMDRPACKET,buffer){
		if(m_buffer){
			m_pkhead.m_pktFlag		= &m_buffer->Buf()[0];
			m_pkhead.m_pktDataSize	= (unsigned short*)&m_buffer->Buf()[1];
			m_pkhead.m_pktSeq		= (unsigned short*)&m_buffer->Buf()[3];
			m_pkhead.m_pktCmdId		= (unsigned short*)&m_buffer->Buf()[5];
			m_pkhead.m_isStructType	= (byte*)&m_buffer->Buf()[7];
			m_pkhead.m_timestamp	= (time_t*)&m_buffer->Buf()[8];
			dataremain              = *m_pkhead.m_pktDataSize;
			rpos                    = sizeof(m_pkhead);	
		}else
			m_pkhead = {0};
	}

	CmdRPacket(const CmdRPacket &o):Packet(RPACKET,o.m_buffer){
		    m_pkhead                = o.m_pkhead;
			dataremain              = *m_pkhead.m_pktDataSize;
			rpos                    = sizeof(m_pkhead);
	}

	CmdRPacket(const CmdWPacket &o);

	CmdRPacket& operator = (const CmdRPacket &o){
		if(this != &o){
			if(m_buffer){
				m_buffer->DecRef();
				m_buffer = NULL;
			}
			if(o.m_buffer){
				m_buffer = o.m_buffer->IncRef();
				rpos = o.rpos;
				m_pkhead = o.m_pkhead;
				dataremain = o.dataremain;
			}else
				m_pkhead = {0};
		}
		return *this;
	}	

	CmdRPacket *Clone(){
		return new CmdRPacket(*this);
	}

	Packet *MakeWritePacket();

	Packet *MakeReadPacket(){
		return Clone();
	}	

	size_t PkLen(){
		return (size_t)*m_pkhead.m_pktDataSize;
	}

	size_t PkTotal(){
		return PkLen() + sizeof(m_pkhead);
	}

	~CmdRPacket(){}

	unsigned short ReadCmd(){
		return *m_pkhead.m_pktCmdId;
	}

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

	unsigned int PeekUint32(){
		if(dataremain < sizeof(unsigned int)) return 0;
		unsigned int ret = m_buffer->ReadUint32(rpos);
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

private:
	stPktHeader m_pkhead;
	size_t      rpos;
	size_t      dataremain;
};

}

#endif
