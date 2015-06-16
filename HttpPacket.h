#ifndef _HTTPPACKET_H
#define _HTTPPACKET_H

#include "Packet.h"

enum{
	URL = 1,
	STATUS,
	HEADER_FIELD,
	HEADER_VALUE,
	BODY,
};

namespace net{

class HttpPacket : public Packet{
private:
	struct Val{
		int    type;
		size_t idx;
		size_t size;
		Val(){
			type = 0;
			idx  = 0;
			size = 0;
		}
	};

public:
	HttpPacket():Packet(HTTPPACKET,new ByteBuffer(4096)),m_data(0){
	}

	HttpPacket(const HttpPacket &o):Packet(HTTPPACKET,o.m_buffer){//,m_header(o.m_header),
	//m_url(o.m_url),m_status(o.m_status),m_body(m_body),m_data(o.m_data){
			m_header = o.m_header;
			m_status = o.m_status;
			m_data   = o.m_data;
			m_url    = o.m_url;
			m_header = o.m_header;
	}

	HttpPacket& operator = (const HttpPacket &o){
		if(&o != this){
			if(m_buffer){
				m_buffer->DecRef();
				m_buffer = o.m_buffer->IncRef();
				m_header = o.m_header;
				m_status = o.m_status;
				m_data   = o.m_data;
				m_url    = o.m_url;
				m_header = o.m_header;
			}
		}	
		return *this;
	} 		

	Packet *Clone(){
		return new HttpPacket(*this);
	}

	Packet *MakeWritePacket(){
		return NULL;
	}

	Packet *MakeReadPacket(){
		return Clone();
	}	

	size_t PkLen(){
		return (size_t)m_data;
	}

	size_t PkTotal(){
		return PkLen();
	}

	void Append(int type,const char *str,size_t len){
		if(type == URL){
			m_url.idx = m_data;
			m_buffer->WriteBin(m_data,(void*)str,len);
			m_data += len;
			m_buffer->WriteUint8(m_data,0);//write '\0';
			m_data += 1;
			m_url.size = len;
		}else if(type == STATUS){
			m_status.idx = m_data;
			m_buffer->WriteBin(m_data,(void*)str,len);
			m_data += len;
			m_buffer->WriteUint8(m_data,0);//write '\0';
			m_data += 1;
			m_status.size = len;		
		}else if(type == BODY){
			m_body.idx = m_data;
			m_buffer->WriteBin(m_data,(void*)str,len);
			m_body.size = len;
			m_data += len;
			printf("body %d\n",len);			
		}else if(type == HEADER_FIELD || type == HEADER_VALUE){
			Val tmp;
			tmp.type = type;
			tmp.idx = m_data;
			m_buffer->WriteBin(m_data,(void*)str,len);
			m_data += len;
			m_buffer->WriteUint8(m_data,0);//write '\0';
			m_data += 1;
			m_header.push_back(tmp);			
		}
	}

	const char *GetUrl(){
		if(m_url.size)
			return (const char*)&m_buffer->Buf()[m_url.idx];
		return NULL;
	}

	const char *GetStatus(){
		if(m_status.size)
			return (const char*)&m_buffer->Buf()[m_status.idx];
		else
			return NULL;
	}

	const char *GetBody(size_t &len){
		len = m_body.size;
		return (const char*)&m_buffer->Buf()[m_body.idx];
	}

	const char *GetHeader(const char *field){
		size_t size = m_header.size();
		for(size_t i = 0;i < size; i += 2){
			if(strcmp((char*)&m_buffer->Buf()[m_header[i].idx],field) == 0 &&
			   i + 1 < size)
			{
				return (const char*)&m_buffer->Buf()[m_header[i+1].idx];
			}
		}
		return NULL;
	}

	std::vector<std::pair<const char*,const char*>> GetHeaders(){
		std::vector<std::pair<const char*,const char*>> ret;
		size_t size = m_header.size();
		for(size_t i = 0;i < size; i += 2){
			ret.push_back(std::make_pair((const char *)&m_buffer->Buf()[m_header[i].idx],
				          (const char*)&m_buffer->Buf()[m_header[i+1].idx]));
		}
		return ret;
	}
	
private:
	std::vector<Val> m_header;
	Val              m_url;
	Val              m_status;
	Val              m_body;
	size_t           m_data;
};

}

#endif