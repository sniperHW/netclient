#ifndef _HTTPDECODER_H
#define _HTTPDECODER_H


#include "HttpPacket.h"
#include "Decoder.h"
#include "http-parser/http_parser.h"

namespace net{

class HttpDecoder : public Decoder{

enum{
	PACKET_COMPLETE = 1,
};	

struct luahttp_parser{
	http_parser base;
	http_parser_settings settings;
	HttpDecoder *decoder;  	
};

public:
	HttpDecoder(int maxsize):m_packet(NULL),status(0),maxsize(maxsize > 65535 ? 65535 : maxsize),m_size(0){
		m_parser.settings.on_message_begin = on_message_begin;
		m_parser.settings.on_url = on_url;
		m_parser.settings.on_status = on_status;
		m_parser.settings.on_header_field = on_header_field;
		m_parser.settings.on_header_value = on_header_value;
		m_parser.settings.on_headers_complete = on_headers_complete;
		m_parser.settings.on_body = on_body;
		m_parser.settings.on_message_complete = on_message_complete;
		m_parser.decoder = this;
		http_parser_init((http_parser*)&m_parser,HTTP_BOTH);		
	}

	virtual ~HttpDecoder(){ if(m_packet) delete m_packet;}

	Packet *unpack(char *buf,size_t pos,size_t size,size_t _,size_t &pklen,int &err){
		Packet *ret = NULL;
		pklen       = 0;
		err         = 0;		
		size_t nparsed = http_parser_execute((http_parser*)&m_parser,&m_parser.settings,&buf[pos],size);
		if(nparsed > 0){
			m_size += nparsed;
			pklen = nparsed;
		}
		if(status == PACKET_COMPLETE){
			status    = 0;
			ret       = m_packet;
			m_packet  = NULL;
			http_parser_init((http_parser*)&m_parser,HTTP_BOTH);			
		}else{
			if(m_size >= maxsize) err = -1;
			if(nparsed != size) err = -1;
		}
		return ret;
	}

private:
	static int on_message_begin (http_parser *_parser){
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		if(decoder->m_packet) return -1;
		decoder->m_packet = new HttpPacket;
		return 0;
	}

	static int on_url(http_parser *_parser, const char *at, size_t length){	
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->m_packet->Append(URL,at,length);
		return 0;
	}

	static int on_status(http_parser *_parser, const char *at, size_t length){
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->m_packet->Append(STATUS,at,length);
		return 0;			
	}

	static int on_header_field(http_parser *_parser, const char *at, size_t length){
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->m_packet->Append(HEADER_FIELD,at,length);
		return 0;			
	}

	static int on_header_value(http_parser *_parser, const char *at, size_t length){
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->m_packet->Append(HEADER_VALUE,at,length);
		return 0;			
	}

	static int on_headers_complete(http_parser *_parser){
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->m_packet->SetMethod(_parser->method);
		return 0;		
	}

	static int on_body(http_parser *_parser, const char *at, size_t length){
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->m_packet->Append(BODY,at,length);
		return 0;					
	}

	static int on_message_complete(http_parser *_parser){	
		HttpDecoder *decoder = ((luahttp_parser*)_parser)->decoder;
		decoder->status = PACKET_COMPLETE;
		return -1;							
	}	

private:
	struct luahttp_parser m_parser;
	HttpPacket           *m_packet;
	int                   status;
	size_t                maxsize;
	size_t  			  m_size;	                

};

}

#endif