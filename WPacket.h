#ifndef _WPACKET_H
#define _WPACKET_H

#include "RPacket.h"
#include <assert.h>

namespace net{

class WPacket : public Packet,public StreamWPacket{

public:
	friend class RPacket;
	//前4个字节用于表示包长度,所以WPacket创建后已经有4个字节的有效数据
	WPacket(unsigned short size = 64):Packet(WPACKET,new ByteBuffer(size)),wpos(0)
	{
		wpos += 4;
	}

	WPacket(const WPacket &o):Packet(WPACKET,o.m_buffer){
		wpos = 0;
	}

	WPacket(const RPacket &o):Packet(WPACKET,o.m_buffer){
		wpos = 0;
	}

	WPacket& operator = (const WPacket &o){
		if(&o != this){
			if(m_buffer){
				m_buffer->DecRef();
				m_buffer = o.m_buffer->IncRef();
			}
			wpos = 0;
		}	
		return *this;
	} 	

	Packet *Clone(){
		return new WPacket(*this);
	}

	Packet *MakeWritePacket(){
		return Clone();
	}

	Packet *MakeReadPacket(){
		return new RPacket(*this);
	}	

	~WPacket(){}

	write_pos GetWritePos() {
		return wpos;
	}

	// rewrite
	void RewriteUint8(write_pos wp,unsigned char v) {
		CopyOnWrite();
		m_buffer->WriteUint8(wp,v);
	}

	void RewriteUint16(write_pos wp,unsigned short v) {
		CopyOnWrite();
		m_buffer->WriteUint16(wp,v);
	}

	void RewriteUint32(write_pos wp,unsigned int v) {
		CopyOnWrite();
		m_buffer->WriteUint32(wp,v);
	}

	void RewriteUint64(write_pos wp,unsigned long long v) {
		CopyOnWrite();
		m_buffer->WriteUint64(wp,v);
	}

	void RewriteDouble(write_pos wp,double v) {
		CopyOnWrite();
		m_buffer->WriteDouble(wp,v);
	}

	void WriteUint8(unsigned char v){
		CopyOnWrite();
		unsigned int size = m_buffer->ReadUint32(0);
		m_buffer->WriteUint8(wpos,v);
		wpos += sizeof(v);
		m_buffer->WriteUint32(0,size+sizeof(v));
	}

	// write
	void WriteUint16(unsigned short v){
		CopyOnWrite();
		unsigned	int size = m_buffer->ReadUint32(0);
		m_buffer->WriteUint16(wpos,v);
		wpos += sizeof(v);
		m_buffer->WriteUint32(0,size+sizeof(v));
	}

	void WriteUint32(unsigned int v){
		CopyOnWrite();
		unsigned int size = m_buffer->ReadUint32(0);
		m_buffer->WriteUint32(wpos,v);
		wpos += sizeof(v);
		m_buffer->WriteUint32(0,size+sizeof(v));
	}

	void WriteUint64(unsigned long long v){
		CopyOnWrite();
		unsigned int size = m_buffer->ReadUint32(0);
		m_buffer->WriteUint64(wpos,v);
		wpos += sizeof(v);
		m_buffer->WriteUint32(0,size+sizeof(v));
	}

	void WriteFloat(float v){
		CopyOnWrite();
		unsigned int size = m_buffer->ReadUint32(0);
		m_buffer->WriteFloat(wpos,v);
		wpos += sizeof(v);
		m_buffer->WriteUint32(0,size+sizeof(v));
	}

	void WriteDouble(double v){
		CopyOnWrite();
		unsigned int size = m_buffer->ReadUint32(0);
		m_buffer->WriteDouble(wpos,v);
		wpos += sizeof(v);
		m_buffer->WriteUint32(0,size+sizeof(v));
	}

	void WriteBin(void *v,size_t len){
		CopyOnWrite();
		unsigned int size = m_buffer->ReadUint32(0);
		m_buffer->WriteUint32(wpos,len);//首先写入长
		m_buffer->WriteBin(wpos+sizeof(uint32_t),v,len);
		wpos += len + sizeof(uint32_t);
		m_buffer->WriteUint32(0,size+4+len);
	}

	void WriteString(const char *v){
		CopyOnWrite();
		WriteBin((void*)v,strlen(v)+1);
	}

	size_t PkLen(){
		return m_buffer->ReadUint32(0);
	}

	size_t PkTotal(){
		return PkLen() + sizeof(uint32_t);
	}	

private:
	void CopyOnWrite(){
		if(wpos == 0){
			ByteBuffer *tmp = new ByteBuffer(*m_buffer);
			m_buffer->DecRef();
			m_buffer = tmp;
			wpos = m_buffer->ReadUint32(0);
		}
	}
	size_t      wpos; 
};


}


#endif
