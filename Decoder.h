#ifndef  _DECODER_H
#define _DECODER_H

#include "Packet.h"
#include "RPacket.h"

namespace net{

class Decoder{
public:
	Decoder(){}
	virtual Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen,int &err) = 0;
	virtual ~Decoder(){};
private:
	Decoder(const Decoder&);
	Decoder& operator = (const Decoder &o);
};

class PacketDecoder : public Decoder{
public:
	Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen,int &err){
		Packet *ret = NULL;
		pklen       = 0;
		err         = 0;
		if(size >= 4){
			int len = (int)chk_ntoh32(*(int*)&buf[pos]);
			if(len <= 0 || (int)(len + sizeof(int)) > (int)max)
				err = -1;
			else{
				len += sizeof(int);
				if(size >= (size_t)len){
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

class RawBinaryDecoder : public Decoder{
public:
	Packet *unpack(char *buf,size_t pos,size_t size,size_t max,size_t &pklen,int &err){
		return NULL;
	}
};

}



#endif