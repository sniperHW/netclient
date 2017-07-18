#include "LuaUtil.h"
#include "LuaPacket.h"
#include "Socket.h"
#include "Reactor.h"
#include "RPacket.h"
#include "WPacket.h"
#include "SysTime.h"
#include "HttpDecoder.h"
#include <signal.h>

namespace net{
static bool init(){
#ifdef _WIN	
   WSADATA wsaData;
   if(WSAStartup(0x0202,&wsaData) != 0)
   {
      WSACleanup();
      return false;
   }
#else
	signal(SIGPIPE,SIG_IGN);
#endif   
   return true;
}
/*
static void clean(){
#ifdef _WIN   
   WSACleanup();
#endif   
}
*/

}

static net::Reactor *g_reactor = NULL;

bool Init(){
	if(g_reactor) return false;
	if(!net::init()) return false;
	g_reactor = new net::Reactor;
	return true;
}


int lua_Close(lua_State *L){
	net::Socket *s = (net::Socket*)lua_touserdata(L,1);
	s->Close();
	return 0;
}

int lua_Run(lua_State *L){
	g_reactor->LoopOnce(lua_tointeger(L,1));
	return 0;
}

int lua_Connect(lua_State *L){
	const char *ip = lua_tostring(L, 1);
	int port       = lua_tointeger(L, 2);
	luaRef cb(L,3);
	net::Socket *s  = new net::Socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	lua_pushboolean(L,(int)s->Connect(g_reactor,ip,port,cb));
	return 1;
}

int lua_Listen(lua_State *L){
	const char *ip = lua_tostring(L, 1);
	int port       = lua_tointeger(L, 2);
	luaRef cb(L,3);
	net::Socket *s  = new net::Socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
	if(0 == s->Listen(g_reactor,ip,port,cb)){
		lua_pushlightuserdata(L,s);
	}else
		lua_pushnil(L);
	return 1;
}

int lua_SendWPacket(lua_State *L){
	net::Socket *s    = (net::Socket*)lua_touserdata(L,1);
	net::Packet *wpk = toLuaPacket(L, 2);
	int  ret; 
	if(lua_gettop(L) == 3){
		luaRef cb(L,3); 
		ret = s->Send(wpk,&cb);
	}else{
		ret = s->Send(wpk,NULL);
	}
	lua_pushboolean(L,ret == 0 ? 1:0);
	return 1;
}

int lua_GetSysTick(lua_State *L){
	lua_pushnumber(L,GetSystemMs64());
	return 1;
}

int lua_Bind(lua_State *L){
	net::Socket  *s    = (net::Socket*)lua_touserdata(L,1);	
	net::Decoder *d    = (net::Decoder*)lua_touserdata(L,2);
	luaRef  cb_packet(L,3);
	luaRef  cb_disconnected(L,4);
	lua_pushboolean(L,s->Bind(g_reactor,d,cb_packet,cb_disconnected));
	return 1;
}

int lua_PacketDecoder(lua_State *L){
	lua_pushlightuserdata(L,new net::PacketDecoder);
	return 1;
}

int lua_HttpDecoder(lua_State *L){
	lua_pushlightuserdata(L,new net::HttpDecoder(lua_tointeger(L,1)));
	return 1;
}

int lua_Socket_Retain(lua_State *L){
	net::Socket *s = (net::Socket*)lua_touserdata(L,1);
	s->IncRef();
	return 0;
}

int lua_Socket_Release(lua_State *L){
	net::Socket *s = (net::Socket*)lua_touserdata(L,1);
	s->DecRef();
	return 0;
}

#define REGISTER_CONST(L,N) do{\
		lua_pushstring(L, #N);\
		lua_pushinteger(L, N);\
		lua_settable(L, -3);\
}while(0)

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)	

extern "C" {
int32_t luaopen_net(lua_State *L)
{
	if(!Init()) return 0;
	lua_newtable(L);
	RegLuaPacket(L);
	REGISTER_FUNCTION("SocketRetain", &lua_Socket_Retain);
	REGISTER_FUNCTION("SocketRelease", &lua_Socket_Release);
	REGISTER_FUNCTION("Connect", &lua_Connect);
	REGISTER_FUNCTION("Listen", &lua_Listen);
	REGISTER_FUNCTION("Close", &lua_Close);
	REGISTER_FUNCTION("Run", &lua_Run);
	REGISTER_FUNCTION("Bind", &lua_Bind);
	REGISTER_FUNCTION("Send", &lua_SendWPacket);
	REGISTER_FUNCTION("GetSysTick", &lua_GetSysTick);
	REGISTER_FUNCTION("PacketDecoder", &lua_PacketDecoder);
	REGISTER_FUNCTION("HttpDecoder", &lua_HttpDecoder);
/*	REGISTER_MODULE(L,"timer",register_timer);
	REGISTER_MODULE(L,"event_loop",register_event_loop);
	REGISTER_MODULE(L,"socket",register_socket);
	REGISTER_MODULE(L,"redis",register_redis);
	REGISTER_MODULE(L,"buffer",register_buffer);
	REGISTER_MODULE(L,"packet",register_packet);
	REGISTER_MODULE(L,"http",register_http);		
	REGISTER_MODULE(L,"signal",register_signum);
	REGISTER_MODULE(L,"log",register_log);
	REGISTER_MODULE(L,"ssl",register_ssl);
	REGISTER_MODULE(L,"time",register_time);		
*/
	return 1;
}
}

/*

bool Reg2Lua(lua_State *L){

	if(!Init()) return false;

	lua_newtable(L);
	RegLuaPacket(L);	
	REGISTER_FUNCTION("SocketRetain", &lua_Socket_Retain);
	REGISTER_FUNCTION("SocketRelease", &lua_Socket_Release);
	REGISTER_FUNCTION("Connect", &lua_Connect);
	REGISTER_FUNCTION("Listen", &lua_Listen);
	REGISTER_FUNCTION("Close", &lua_Close);
	REGISTER_FUNCTION("Run", &lua_Run);
	REGISTER_FUNCTION("Bind", &lua_Bind);
	REGISTER_FUNCTION("Send", &lua_SendWPacket);
	REGISTER_FUNCTION("GetSysTick", &lua_GetSysTick);
	REGISTER_FUNCTION("PacketDecoder", &lua_PacketDecoder);
	REGISTER_FUNCTION("HttpDecoder", &lua_HttpDecoder);
	lua_setglobal(L,"C");
	return true;
}*/