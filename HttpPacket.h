#ifndef _HTTPPACKET_H
#define _HTTPPACKET_H

#include "Packet.h"
#include "LuaUtil.h"

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

public:
	HttpPacket():Packet(HTTPPACKET,NULL),m_method(-1),m_bodysize(0){
	}

	~HttpPacket(){}

	HttpPacket(const HttpPacket &o):Packet(HTTPPACKET,NULL){
			m_header_field = o.m_header_field;
			m_header_value = o.m_header_value;
			m_status = o.m_status;
			m_url    = o.m_url;
			m_bodysize = o.m_bodysize;
			if(o.m_buffer){
				m_buffer = o.m_buffer;
				m_buffer->IncRef();
			}
			m_method = o.m_method;
	}

	HttpPacket& operator = (const HttpPacket &o){
		if(&o != this){
			m_header_field = o.m_header_field;
			m_header_value = o.m_header_value;
			m_status = o.m_status;
			m_url    = o.m_url;
			m_method = o.m_method;
			m_bodysize = o.m_bodysize;			
			if(o.m_buffer){
				m_buffer = o.m_buffer;
				m_buffer->IncRef();
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
		return 0;
	}

	size_t PkTotal(){
		return 0;
	}

	void SetMethod(int method){
		m_method = method;
	}

	int  GetMethod(){
		return m_method;
	}

	void Append(int type,const char *str,size_t len){
		if(type == URL){
			for(size_t i = 0;i < len;++i)
				m_url.push_back(str[i]);
		}else if(type == STATUS){
			for(size_t i = 0;i < len;++i)
				m_status.push_back(str[i]);		
		}else if(type == BODY){
			if(!m_buffer) m_buffer = new ByteBuffer(1024);
			m_buffer->WriteBin(m_bodysize,(void*)(str),len);
			m_bodysize += len;						
		}else if(type == HEADER_FIELD){
			if(m_header_field.size() == m_header_value.size()){
				m_header_field.push_back(std::string(""));
			}
			std::string *ptr = &m_header_field[m_header_field.size()-1];
			for(size_t i = 0;i < len;++i)
				ptr->push_back(str[i]);			
		}else if(type == HEADER_VALUE){
			if(m_header_field.size() != m_header_value.size()){
				m_header_value.push_back(std::string(""));
			}			
			std::string *ptr = &m_header_value[m_header_value.size()-1];
			for(size_t i = 0;i < len;++i)
				ptr->push_back(str[i]);				
		}			
	}

	const char *GetUrl(){
		return m_url.c_str();
	}

	const char *GetStatus(){
		return m_status.c_str();
	}

	const char *GetBody(size_t &len){
		if(!m_buffer) return NULL;
		len = m_bodysize;
		return &m_buffer->Buf()[0];
	}

	void PushHeaders(lua_State *L){
		lua_newtable(L);
		size_t size = m_header_value.size();
		for(size_t i = 0; i < size; ++i){
			lua_pushstring(L, m_header_field[i].c_str());
			lua_pushstring(L, m_header_value[i].c_str());
			lua_rawset(L, -3);
		}
	}
	
private:
	std::vector<std::string> m_header_field;
	std::vector<std::string> m_header_value;	
	std::string              m_url;
	std::string              m_status;
	int                      m_method;
	size_t                   m_bodysize;
};

}

#endif