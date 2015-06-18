#ifndef _SOCKET_H
#define _SOCKET_H

#ifdef _WIN

#include <winsock2.h>
#include <WinBase.h>
#include <Winerror.h>
#include <stdio.h>
#include <WS2tcpip.h>

#define INVAILD_FD INVALID_SOCKET

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) expression
#endif

#else

#include    <unistd.h>
#include    <sys/select.h>
#include	<sys/types.h>	
#include	<sys/socket.h>	
#include	<sys/time.h>	
#include	<time.h>		
#include	<netinet/in.h>	
#include	<arpa/inet.h>	
#include	<errno.h>
#include	<fcntl.h>		
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	
#include	<sys/uio.h>		
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		
#include    <net/if.h>
#include    <sys/ioctl.h>
#include    <netinet/tcp.h>
#include    <fcntl.h>
#include    <stdint.h>

#ifdef TEMP_FAILURE_RETRY
#undef  TEMP_FAILURE_RETRY
#endif
#define TEMP_FAILURE_RETRY(expression)\
	({ long int __result;\
	do __result = (long int)(expression);\
	while(__result == -1L&& errno == EINTR);\
	__result;})


typedef int SOCKET;
#define INVAILD_FD -1
#endif

#include "LuaUtil.h"
#include "Packet.h"
#include "RPacket.h"
#include "WPacket.h"
#include "dlist.h"
#include "Decoder.h"
#include <list>


#define EV_READ 0x1
#define EV_WRITE (0x1 << 1)

namespace net{

enum{
	listening = 1,
	connecting,
	establish,
	timeout,
	closeing,
};

class WPacket;
class Reactor;
class RPacket;
class Socket;

class Socket:public dnode{
	friend class Reactor;
	friend void do_cb_newclient(Socket *s,Socket *client);
	friend void do_cb_connect(Socket *s,int success);
	friend void do_cb_packet(Socket *s,Packet*);
	friend void do_cb_disconnected(Socket *s);		
public:
	Socket(int family,int type,int protocol);
	Socket(SOCKET fd);
	bool SetNonBlock();
	int  Send(Packet*,luaRef*);
	bool Bind(Reactor *reactor,Decoder *,luaRef&,luaRef&);
	void Close();
	int  Event(){return event;}
	int  State(){return state;}
	bool  Listen(Reactor*,const char *ip,int port,luaRef&);
	bool  Connect(Reactor *reactor,const char *ip,int port,luaRef&);
	SOCKET Fd(){return fd;}
	void SetUd(void *ud){this->ud = ud;}
	void *GetUd(){return ud;}
	void IncRef(){
#ifdef _WIN		
		InterlockedIncrement(&refCount);
#else
		__sync_add_and_fetch(&refCount,1);
#endif
	}

	void DecRef(){
#ifdef _WIN		
		if(InterlockedDecrement(&refCount) <= 0)
#else
		if(__sync_sub_and_fetch(&refCount,1) <=0)	
#endif
			delete this;		
	}

private:
	Socket(const Socket&);
	Socket& operator = (const Socket &o);
	~Socket(){ if(decoder) delete decoder;} 	
	int  rawSend();
	void onReadAct();
	void onWriteAct();
	void doAccept();
	void doConnect();
	void unpack();

private:

	struct stSendFinish{
		luaRef         cb;
		Packet        *packet;
		stSendFinish(Packet *p,luaRef &r):cb(r),packet(p)
		{}		
	};

	SOCKET        fd;
	static const  int recvbuf_size = 65535;
	static const  int maxpacket_size = 65535;
	char   		  recvbuf[recvbuf_size];
	Reactor      *reactor;
	bool    	  writeable;
	volatile      long refCount;
	int           state;
	size_t        wpos;
	size_t        upos;
	char          unpackbuf[maxpacket_size];
	int           event;
	void         *ud;	
	std::list<Packet*>        sendlist;
	std::list<stSendFinish>   finishcb_list;
	luaRef        cb_connect;
	luaRef        cb_new_client;
	luaRef        cb_disconnected;
	luaRef        cb_packet;
	Decoder      *decoder;   	
};

}//end namespace net
#endif
