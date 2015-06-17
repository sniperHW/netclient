#ifndef  _DECODER_H
#define _DECODER_H

#include "Packet.h"
#include "RPacket.h"
#include "CmdRPacket.h"

namespace net{

class Decoder{
public:
	Decoder(){}
	virtual Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen) = 0;
	virtual ~Decoder(){};
private:
	Decoder(const Decoder&);
	Decoder& operator = (const Decoder &o);
};

class PacketDecoder : public Decoder{
public:
	Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen){
		Packet *ret = NULL;
		size_t  data = size-pos;
		pklen       = 0;
		if(pos < size && data >= 4){
			int len = *(int*)&buf[pos];
			if(len <= 0 || len + sizeof(int) > (int)max)
				pklen = -1;
			else{
				len += sizeof(int);
				if(data >= (size_t)len){
					ByteBuffer *b = new ByteBuffer(len);
					b->WriteBin(0,(void*)&buf[pos],len);
					ret   = new RPacket(b);
					pklen = len;
					b->DecRef();
				}
			}
		}			
		return ret;
	}
};


class CmdPacketDecoder : public Decoder{
public:
	Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen){
		Packet *ret = NULL;	
		stPktHeader header;
		pklen = 0;
		if(pos < size && size-pos >= (int)sizeof(header)){
			//header  = (stPktHeader*)&unpackbuf[pos];
			//header.m_pktFlag		= &unpackbuf[0];
			header.m_pktDataSize	= (unsigned short*)&buf[1];
			//header.m_pktSeq		= (unsigned short*)&unpackbuf[3];
			//header.m_pktCmdId		= (unsigned short*)&unpackbuf[5];
			//header.m_isStructType	= (byte*)&unpackbuf[7];
			//header.m_timestamp	= (time_t *)&unpackbuf[8];
			size_t len = (size_t)*header.m_pktDataSize + sizeof(header);
			if(len <= 0 || len > (int)max)
				pklen = -1;
			else if(size-pos >= len){
				ByteBuffer *b = new ByteBuffer(len);
				b->WriteBin(0,(void*)&buf[pos],len);
				ret   = new CmdRPacket(b);
				pklen = len;
				b->DecRef();
			}
		}
		return ret;
	}
};

class RawBinaryDecoder : public Decoder{
public:
	Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen){
		return NULL;
	}
};

}



#endif