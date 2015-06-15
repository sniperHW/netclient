#ifndef _CMDWPACKET_H
#define _CMDWPACKET_H

#include "CmdRPacket.h"
#include <assert.h>

namespace net{

//特定游戏的包结构

class CmdWPacket : public Packet,public StreamWPacket{

public:
	friend class CmdRPacket;
	CmdWPacket(unsigned short size = 64):Packet(CMDWPACKET,new ByteBuffer(size))
	{
		m_pkhead.m_pktFlag		= &m_buffer->Buf()[0];
		m_pkhead.m_pktDataSize	= (unsigned short*)&m_buffer->Buf()[1];
		m_pkhead.m_pktSeq		= (unsigned short*)&m_buffer->Buf()[3];
		m_pkhead.m_pktCmdId		= (unsigned short*)&m_buffer->Buf()[5];
		m_pkhead.m_isStructType	= (byte*)&m_buffer->Buf()[7];
		m_pkhead.m_timestamp	= (time_t *)&m_buffer->Buf()[8];
	}

	CmdWPacket(const CmdWPacket &o):Packet(CMDWPACKET,o.m_buffer),m_cpConstr(true){
		if(m_buffer) m_pkhead = o.m_pkhead;
		else m_pkhead = {0};
	}

	CmdWPacket(const CmdRPacket &o):Packet(CMDWPACKET,o.m_buffer),m_cpConstr(true){
		if(m_buffer) m_pkhead = o.m_pkhead;
		else m_pkhead = {0};
	}

	Packet *Clone(){
		return new CmdWPacket(*this);
	}

	Packet *MakeWritePacket(){
		return Clone();
	}

	Packet *MakeReadPacket(){
		return new CmdRPacket(*this);
	}		

	~CmdWPacket(){}

	size_t GetWritePos(){
		return PkTotal();
	}

	void WriteCmd(unsigned short cmd){
		CopyOnWrite();
		*m_pkhead.m_pktCmdId = cmd;
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
		m_buffer->WriteUint8(PkTotal(),v);
		*m_pkhead.m_pktDataSize += sizeof(v);
	}

	// write
	void WriteUint16(unsigned short v){
		CopyOnWrite();
		m_buffer->WriteUint16(PkTotal(),v);
		*m_pkhead.m_pktDataSize += sizeof(v);
	}

	void WriteUint32(unsigned int v){
		CopyOnWrite();
		m_buffer->WriteUint32(PkTotal(),v);
		*m_pkhead.m_pktDataSize += sizeof(v);
	}

	void WriteUint64(unsigned long long v){
		CopyOnWrite();
		m_buffer->WriteUint64(PkTotal(),v);
		*m_pkhead.m_pktDataSize += sizeof(v);
	}

	void WriteFloat(float v){
		CopyOnWrite();
		m_buffer->WriteFloat(PkTotal(),v);
		*m_pkhead.m_pktDataSize += sizeof(v);
	}

	void WriteDouble(double v){
		CopyOnWrite();
		m_buffer->WriteDouble(PkTotal(),v);
		*m_pkhead.m_pktDataSize += sizeof(v);
	}

	void WriteBin(void *v,size_t len){
		CopyOnWrite();
		WriteUint32(len);
		m_buffer->WriteBin(PkTotal(),v,len);
		*m_pkhead.m_pktDataSize += len;
	}

	void WriteString(const char *v){
		CopyOnWrite();
		WriteBin((void*)v,strlen(v)+1);
	}

	size_t PkLen(){
		return (size_t)*m_pkhead.m_pktDataSize;
	}

	size_t PkTotal(){
		return PkLen() + sizeof(m_pkhead);
	}	

private:
	CmdWPacket& operator = (const CmdWPacket &o){
		if(&o != this){
			if(m_buffer){
				m_buffer->DecRef();
				m_buffer = NULL;
			}
			if(o.m_buffer){
				m_buffer = o.m_buffer->IncRef();
				m_pkhead = o.m_pkhead;
			}else
				m_pkhead = {0};		
			m_cpConstr = true;
		}	
		return *this;
	} 

	void CopyOnWrite(){
		if(m_cpConstr){
			assert(m_buffer);
			ByteBuffer *tmp 		= new ByteBuffer(*m_buffer);
			m_buffer->DecRef();
			m_buffer 				= tmp;
			m_pkhead.m_pktFlag		= &m_buffer->Buf()[0];
			m_pkhead.m_pktDataSize	= (unsigned short*)&m_buffer->Buf()[1];
			m_pkhead.m_pktSeq		= (unsigned short*)&m_buffer->Buf()[3];
			m_pkhead.m_pktCmdId		= (unsigned short*)&m_buffer->Buf()[5];
			m_pkhead.m_isStructType	= (byte*)&m_buffer->Buf()[7];
			m_pkhead.m_timestamp	= (time_t *)&m_buffer->Buf()[8];
			m_cpConstr 				= false;
		}
	}
	stPktHeader m_pkhead;
	bool        m_cpConstr; 
};


}


#endif
