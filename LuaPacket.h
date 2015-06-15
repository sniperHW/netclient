#ifndef _LUAPACKET_H
#define _LUAPACKET_H

extern "C"{
#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>
}

#include "Socket.h"

void RegLuaPacket(lua_State *L);
void push_luaPacket(lua_State *L,net::Packet *rpk);
net::Packet *toLuaPacket(lua_State *L,int index);

#endif // _LUAPACKET_H