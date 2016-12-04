#ifndef _PACKET_H
#define _PACKET_H

#include "ByteBuffer.h"

enum{
	WPACKET = 1,
	RPACKET,
	HTTPPACKET,
	RAWBINARY,
};

namespace net{

class Packet{
public:
	Packet(int type,ByteBuffer *buff):m_type(type),m_buffer(NULL){
		if(buff){
			m_buffer = buff->IncRef();		
		}
	}

	virtual Packet *Clone() = 0;

	virtual Packet *MakeWritePacket() = 0;

	virtual Packet *MakeReadPacket()  = 0;

	virtual ~Packet(){
		if(m_buffer) m_buffer->DecRef();
	}
	int Type() const{return m_type;}

	virtual size_t PkLen() = 0;

	virtual size_t PkTotal() = 0;

	ByteBuffer *Buffer() {return m_buffer;}

	Packet(const Packet&);
	Packet& operator = (const Packet&);

protected:
	int         m_type;
	ByteBuffer *m_buffer;	
};



class StreamRPacket{
public:
	virtual unsigned char ReadUint8() = 0;

	virtual unsigned short ReadUint16() = 0;

	virtual unsigned int ReadUint32() = 0;

	virtual unsigned short PeekUint16() = 0;

	virtual unsigned int PeekUint32() = 0;
	
	virtual char ReadInt8() = 0;

	virtual short ReadInt16() = 0;

	virtual int ReadInt32() = 0;

	virtual unsigned long long ReadUint64() = 0;

	virtual float ReadFloat() = 0;

	virtual double ReadDouble() = 0;

    virtual void *ReadBin(size_t &len) = 0;

	virtual const char *ReadString() = 0;
};

typedef size_t write_pos;

class StreamWPacket{
public:
	
	virtual size_t GetWritePos() = 0;

	// rewrite
	virtual void RewriteUint8(write_pos wp,unsigned char v) = 0;

	virtual void RewriteUint16(write_pos wp,unsigned short v) = 0;

	virtual void RewriteUint32(write_pos wp,unsigned int v) = 0;

	virtual void RewriteUint64(write_pos wp,unsigned long long v) = 0;

	virtual void RewriteDouble(write_pos wp,double v) = 0;

	virtual void WriteUint8(unsigned char v) = 0;

	// write
	virtual void WriteUint16(unsigned short v) = 0;

	virtual void WriteUint32(unsigned int v) = 0;

	virtual void WriteUint64(unsigned long long v) = 0;

	virtual void WriteFloat(float v) = 0;

	virtual void WriteDouble(double v) = 0;

	virtual void WriteBin(void *v,size_t len) = 0;

	virtual void WriteString(const char *v) = 0;
};	


}



#endif