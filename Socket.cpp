#include "Socket.h"
#include "Reactor.h"
#include "SysTime.h"
#include "LuaPacket.h"
namespace net{

Socket::Socket(int family,int type,int protocol):reactor(NULL),
	writeable(true),refCount(1),state(0),wpos(0),upos(0),event(0),ud(NULL),
	cb_connect(NULL,0),cb_new_client(NULL,0),
	cb_disconnected(NULL,0),cb_packet(NULL,0),decoder(NULL)
{
	fd = ::socket(family,type,protocol);
	if(fd < 0) exit(0);
}

Socket::Socket(SOCKET fd):fd(fd),reactor(NULL),
	writeable(true),refCount(1),state(0),wpos(0),upos(0),event(0),ud(NULL),	
	cb_connect(NULL,0),cb_new_client(NULL,0),
	cb_disconnected(NULL,0),cb_packet(NULL,0),decoder(NULL)
{}

bool  Socket::Listen(Reactor *reactor,const char *ip,int port,luaRef &cb)
{
	if(!reactor || !ip || !cb.GetLState()) return false;
	struct sockaddr_in servaddr;
	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port = htons(port);


	int yes = 1;

	::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	if(::bind(fd,(const sockaddr *)&servaddr,sizeof(servaddr)) < 0)
		return false;

	if(::listen(fd,256) < 0)
		return false;
	SetNonBlock();
	cb_new_client = cb;
	reactor->Add(this,EV_READ);
	state = listening;
	return true;
}

bool Socket::Connect(Reactor *reactor,const char *host,int port,luaRef &cb)
{
	if(!reactor || !host || !cb.GetLState()) return false;
	
	struct sockaddr_in remote;	
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);

	hostent *phostent = gethostbyname(host);
	if(phostent)
		remote.sin_addr.s_addr = *((unsigned long*)phostent->h_addr_list[0]); 
	else
		remote.sin_addr.s_addr = inet_addr(host);
	if(!SetNonBlock()){ 
		printf("Connect SetNonBlock error\n");
		return false;
	}
	cb_connect = cb;
#ifdef _WIN
	if(::connect(fd,(const sockaddr *)&remote,sizeof(remote)) != SOCKET_ERROR){
#else
	if(::connect(fd,(const sockaddr *)&remote,sizeof(remote)) == 0){
#endif
		state = establish;
		do_cb_connect(this,1);
		return true;
	}else{
#ifdef _WIN
		if(WSAGetLastError() != WSAEWOULDBLOCK){
#else		
		if(errno != EINPROGRESS){
#endif	
			do_cb_connect(this,0);
		}
		
	}
	reactor->Add(this,EV_WRITE);
	state = connecting;
	return true;
}


void Socket::doAccept(){
	for(;;){
		SOCKET clientfd;
		struct sockaddr sa;	
#ifdef _WIN		
		int   addrlen = sizeof(sa);
#else
		socklen_t addrlen = sizeof(sa);
#endif	
		if((clientfd = TEMP_FAILURE_RETRY(::accept(fd,&sa,&addrlen))) == INVAILD_FD){
			return;
		}
		Socket *client = new Socket(clientfd);
		client->state = establish;
		do_cb_newclient(this,client);
	}
}

void Socket::unpack(){

	size_t  pos    = 0;
	size_t  size   = (size_t)upos - pos;
	size_t  pklen;
	Packet *packet;
	int     err;
	do{
		packet = this->decoder->unpack(unpackbuf,pos,size,maxpacket_size,pklen,err);
		if(err){
			Close();
			return;
		}

		if(pklen > 0){
			pos += pklen;
			size = upos - pos;
		}
		if(packet){
			do_cb_packet(this,packet);
			delete packet;
		}else
			break;
	}while(size && state == establish);
	if(size)
		memmove(unpackbuf,&unpackbuf[pos],size);
	upos = size;
}

void Socket::onReadAct()
{
	if(state == listening)
		doAccept();
	else if(state == connecting)
		doConnect();
	else if(state == establish){
		int n = TEMP_FAILURE_RETRY(::recv(fd,recvbuf,recvbuf_size,0));
		if(n == 0){
			Close();	
#ifdef _WIN				
		}else if(n == SOCKET_ERROR){
			if(WSAGetLastError() != WSAEWOULDBLOCK)
#else
	    }else if(n < 0){
			if(errno != EWOULDBLOCK || errno != EAGAIN)
#endif	
				Close();
		}else{
			memcpy(&unpackbuf[upos],recvbuf,n);
			upos += n;
			unpack();
		}
	}
}

void Socket::doConnect()
{
    int err = 0;
    socklen_t len = sizeof(err);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len) == -1)   
        state = 0;
    if(err)
        state = 0;
	if(state != 0)
		state = establish;
	reactor->Remove(this,EV_WRITE);
	do_cb_connect(this,state == establish?1:0);
}


int  Socket::rawSend(){
	while(writeable && !sendlist.empty()){
		Packet *wpk = sendlist.front();	
		char *buf = (char *)wpk->Buffer()->ReadBin(wpos);
		int len = wpk->PkTotal()-wpos;
		int n = TEMP_FAILURE_RETRY(::send(fd,&buf[wpos],len,0));
		if(n == 0){
			writeable = false;
			return -1;		
#ifdef _WIN			
		}else if(n == SOCKET_ERROR){
			if(WSAGetLastError() != WSAEWOULDBLOCK){
#else
	    }else if(n < 0){
			if(errno != EWOULDBLOCK || errno != EAGAIN){
#endif		
				writeable = false;
				return -1;
			}
			else{
				writeable = false;
				if(!(event & EV_WRITE))
					reactor->Add(this,EV_WRITE);
				return 0;
			}
		}else{
			if(n == len){
				sendlist.pop_front();
				delete wpk;
				wpos = 0;

				if(!finishcb_list.empty()){
					stSendFinish stCb = finishcb_list.front();
					if(stCb.packet == wpk){
						finishcb_list.pop_front();
						lua_State *L = stCb.cb.GetLState();
						int oldtop = lua_gettop(L);
						lua_rawgeti(L, LUA_REGISTRYINDEX, stCb.cb.GetIndex());
						if(0 != lua_pcall(L, 0, 0, 0))
							printf("%s\n",lua_tostring(L,-1));
						lua_settop(L, oldtop);

						if(state == closeing)
							return 0;
					}
				}


			}else{
				wpos += len;		
			}
		}			
	}
	return 0;
}

void Socket::onWriteAct()
{
	if(state == connecting){
		doConnect();
	}else if(state == establish){
		writeable = true;
		if(-1 == rawSend()){
			Close();
		}
	}
}

void  Socket::Close()
{
	if(state != closeing){
		state = closeing;
#if _WIN		
		::closesocket(fd);
#else	
		::close(fd);
#endif
		while(!sendlist.empty()){
			delete sendlist.front();
			sendlist.pop_front();
		}

		if(reactor)
			reactor->Remove(this,EV_WRITE|EV_READ);
		if(cb_disconnected.GetLState()) 
			do_cb_disconnected(this);
		DecRef();
	}
}

int  Socket::Send(Packet *wpk,luaRef *cb){
	if(state != establish) return -1;
	wpk = wpk->Clone();
	sendlist.push_back(wpk);
	if(cb){
		stSendFinish stCb(wpk,*cb);
		finishcb_list.push_back(stCb);
	}
	return rawSend();
}

bool Socket::Bind(Reactor *reactor,Decoder *decoder,luaRef &cb1,luaRef &cb2){
	if(state == establish){
		this->reactor = reactor;
		this->reactor->Add(this,EV_READ);
		cb_packet = cb1;
		cb_disconnected = cb2;
		this->decoder = decoder ? decoder: new RawBinaryDecoder();
		return true;
	}
	return false;
}

bool Socket::SetNonBlock(){
		int ret;
#ifdef _WIN
		int ioctlvar;

		ioctlvar = 1;
		ret = ioctlsocket(fd, FIONBIO, (unsigned long*)&ioctlvar);
#else
		ret = fcntl(fd, F_SETFL, O_NONBLOCK | O_RDWR);
#endif
		if (ret != 0) {
#ifdef _WIN
			errno = WSAGetLastError();
#endif
			return false;
		}
		return true;
}

void do_cb_newclient(Socket *s,Socket *client){
	if(client){
		lua_State *L = s->cb_new_client.GetLState();
		int oldtop = lua_gettop(L);
		lua_rawgeti(L, LUA_REGISTRYINDEX, s->cb_new_client.GetIndex());
		lua_pushlightuserdata(L,client);
		if(0 != lua_pcall(L, 1, 0, 0))
			printf("%s\n",lua_tostring(L,-1));
		lua_settop(L, oldtop);
	}	
}

void do_cb_connect(Socket *s,int success){
	lua_State *L = s->cb_connect.GetLState();
	int oldtop = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, s->cb_connect.GetIndex());
	if(success)
		lua_pushlightuserdata(L,s);
	else
		lua_pushnil(L);
	lua_pushboolean(L,success);
	if(0 != lua_pcall(L, 2, 0, 0))
		printf("%s\n",lua_tostring(L,-1));
	lua_settop(L, oldtop);
	if(!success)
		s->Close();	
}

void do_cb_packet(Socket *s,Packet *rpk){
	lua_State *L = s->cb_packet.GetLState();
	int oldtop = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, s->cb_packet.GetIndex());
	lua_pushlightuserdata(L,s);
	push_luaPacket(L,rpk);
	if(0 != lua_pcall(L, 2, 0, 0))
		printf("%s\n",lua_tostring(L,-1));
	lua_settop(L, oldtop);		
}


void do_cb_disconnected(Socket *s){
	lua_State *L = s->cb_disconnected.GetLState();
	int oldtop = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, s->cb_disconnected.GetIndex());
	lua_pushlightuserdata(L,s);
	if(0 != lua_pcall(L, 1, 0, 0))
		printf("%s\n",lua_tostring(L,-1));
	lua_settop(L, oldtop);
}

}
