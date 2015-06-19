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
static inline bool init(){
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

static inline void clean(){
#ifdef _WIN   
   WSACleanup();
#endif   
}

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

int lua_CmdPacketDecoder(lua_State *L){
	lua_pushlightuserdata(L,new net::CmdPacketDecoder);
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

char* U2G(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len+1];
	memset(wstr, 0, len+1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len+1];
	memset(str, 0, len+1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if(wstr) delete[] wstr;
	return str;
}

//GB2312到UTF-8的转换
char* G2U(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len+1];
	memset(wstr, 0, len+1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len+1];
	memset(str, 0, len+1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	if(wstr) delete[] wstr;
	return str;
}

int lua_U2G(lua_State *L){
	const char *str = lua_tostring(L,1);
	char *gstr      = U2G(str);
	if(gstr){
		lua_pushstring(L,gstr);
		delete[] gstr;
	}else
		lua_pushnil(L); 
	return 1;
}

int lua_G2U(lua_State *L){
	const char *str = lua_tostring(L,1);
	char *ustr      = G2U(str);
	if(ustr){
		lua_pushstring(L,ustr);
		delete[] ustr;
	}else
		lua_pushnil(L); 
	return 1;
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

bool Reg2Lua(lua_State *L){

	if(!Init()) return false;

	lua_newtable(L);
	RegLuaPacket(L);
	REGISTER_FUNCTION("Utf82Gbk", &lua_U2G);
	REGISTER_FUNCTION("Gbk2Utf8", &lua_G2U);	
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
	REGISTER_FUNCTION("CmdPacketDecoder", &lua_CmdPacketDecoder);
	REGISTER_FUNCTION("HttpDecoder", &lua_HttpDecoder);
	lua_setglobal(L,"C");
	return true;
}