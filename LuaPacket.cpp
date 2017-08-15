#include "LuaPacket.h"
#include "assert.h"
#include "RPacket.h"
#include "WPacket.h"
#include "HttpPacket.h"
#include "RawBinPacket.h"
#include <math.h>

enum{
	L_TABLE = 1,
	L_STRING,
	L_BOOL,
	L_FLOAT,
	L_DOUBLE,
	L_UINT8,
	L_UINT16,
	L_UINT32,
	L_INT8,
	L_INT16,
	L_INT32,
	L_INT64,		
};

typedef struct{
	 net::Packet* packet;
}lua_packet,*lua_packet_t;

#define LUARPACKET_METATABLE    "luarpacket_metatable"
#define LUAWPACKET_METATABLE    "luawpacket_metatable"
#define LUAHTTPPACKET_METATABLE "luahttppacket_metatable"
#define LUARAWPACKET_METATABLE  "luarawpacket_metatable"

#define VAILD_KEY_TYPE(TYPE) (TYPE == LUA_TSTRING || TYPE == LUA_TNUMBER)
#define VAILD_VAILD_TYPE(TYPE) (TYPE == LUA_TSTRING || TYPE == LUA_TNUMBER || TYPE == LUA_TTABLE || TYPE == LUA_TBOOLEAN)

static inline void luabin_pack_string(net::StreamWPacket* wpk,lua_State *L,int index){
	wpk->WriteUint8(L_STRING);
	size_t len;
	const char *data = lua_tolstring(L,index,&len);
	wpk->WriteBin((void*)data, len);
}

static void memrevifle(void *ptr, size_t len) {
    unsigned char   *p = (unsigned char *)ptr,
                    *e = (unsigned char *)p+len-1,
                    aux;
    int test = 1;
    unsigned char *testp = (unsigned char*) &test;

    if (testp[0] == 0) return; /* Big endian, nothing to do. */
    len /= 2;
    while(len--) {
        aux = *p;
        *p = *e;
        *e = aux;
        p++;
        e--;
    }
}

static void encode_double(net::StreamWPacket* wpk, double d) {
    unsigned char b[8];
    float f = d;
     if (d == (double)f) {
        memcpy(b,&f,4);
        memrevifle(b,4);
        wpk->WriteUint8(L_FLOAT);
        wpk->WriteFloat(*((float*)b));
    } else if (sizeof(d) == 8) {
        memcpy(b,&d,8);
        memrevifle(b,8);
        wpk->WriteUint8(L_DOUBLE);
        wpk->WriteDouble(*((double*)b));
        memrevifle(b,8);   
    }
}

static inline void luabin_pack_number(net::StreamWPacket* wpk,lua_State *L,int index){
	lua_Number v = lua_tonumber(L,index);
	if(v != (lua_Integer)v){
		encode_double(wpk,v);
	}else{
		if((long long)v > 0){
			unsigned long long _v = (unsigned long long)v;
			if(_v <= 0xFF){
				wpk->WriteUint8(L_UINT8);
				wpk->WriteUint8((unsigned char)_v);
			}else if(_v <= 0xFFFF){
				wpk->WriteUint8(L_UINT16);
				wpk->WriteUint16((unsigned short)_v);
			}else if(_v <= 0xFFFFFFFF){
				wpk->WriteUint8(L_UINT32);
				wpk->WriteUint32((unsigned int)_v);
			}else{
				wpk->WriteUint8(L_INT64);
				wpk->WriteUint64((unsigned long long)_v);
			}
		}else{
			long long _v = (long long)v;
			if(_v >= 0x80){
				wpk->WriteUint8(L_INT8);
				wpk->WriteUint8((unsigned char)_v);
			}else if(_v >= 0x8000){
				wpk->WriteUint8(L_INT16);
				wpk->WriteUint16((unsigned short)_v);
			}else if(_v < 0x80000000){
				wpk->WriteUint8(L_INT32);
				wpk->WriteUint32((unsigned int)_v);
			}else{
				wpk->WriteUint8(L_INT64);
				wpk->WriteUint64((unsigned long long)_v);
			}
		}
	}
}

static inline void luabin_pack_boolean(net::StreamWPacket* wpk,lua_State *L,int index){
	wpk->WriteUint8(L_BOOL);
	int value = lua_toboolean(L,index);
	wpk->WriteUint8(value);
}


static int luabin_pack_table(net::StreamWPacket* wpk,lua_State *L,int index){
	if(0 != lua_getmetatable(L,index)){
		lua_pop(L,1);
		return -1;
	}
	wpk->WriteUint8(L_TABLE);
	net::write_pos wpos = wpk->GetWritePos();
	wpk->WriteUint32(0);
	int ret = 0;
	int c = 0;
	int top = lua_gettop(L);
	lua_pushnil(L);
	do{		
		if(!lua_next(L,index - 1)){
			break;
		}
		int key_type = lua_type(L, -2);
		int val_type = lua_type(L, -1);
		if(!VAILD_KEY_TYPE(key_type)){
			lua_pop(L,1);
			continue;
		}
		if(!VAILD_VAILD_TYPE(val_type)){
			lua_pop(L,1);
			continue;
		}
		if(key_type == LUA_TSTRING)
			luabin_pack_string(wpk,L,-2);
		else
			luabin_pack_number(wpk,L,-2);

		if(val_type == LUA_TSTRING)
			luabin_pack_string(wpk,L,-1);
		else if(val_type == LUA_TNUMBER)
			luabin_pack_number(wpk,L,-1);
		else if(val_type == LUA_TBOOLEAN)
			luabin_pack_boolean(wpk,L,-1);
		else if(val_type == LUA_TTABLE){
				if(0 != (ret = luabin_pack_table(wpk,L,-1)))
					break;
		}else{
			ret = -1;
			break;
		}
		lua_pop(L,1);
		++c;
	}while(1);
	lua_settop(L,top);
	if(0 == ret){
		wpk->RewriteUint32(wpos, c);
	}						
	return ret;
}

static inline void un_pack_boolean(net::StreamRPacket *rpk,lua_State *L){
	int n = rpk->ReadUint8();
	lua_pushboolean(L,n);
}

static inline void un_pack_number(net::StreamRPacket *rpk,lua_State *L,int type){
	double n;
	switch(type){
		case L_FLOAT:
		case L_DOUBLE:
		{
			if(type == L_FLOAT) {
				float f = rpk->ReadFloat();
				memrevifle(&f,4);
				n = (double)f;
			} else {
				double d = rpk->ReadDouble();
				memrevifle(&d,8);
				n = (double)d;				
			}
			break;
		}
		case L_UINT8:{
			n = (double)rpk->ReadUint8();
			break;
		}
		case L_UINT16:{
			n = (double)rpk->ReadUint16();
			break;
		}
		case L_UINT32:{
			n = (double)rpk->ReadUint32();
			break;
		}
		case L_INT8:{
			n = (double)((char)rpk->ReadUint8());
			break;
		}
		case L_INT16:{
			n = (double)((short)rpk->ReadUint16());
			break;
		}
		case L_INT32:{
			n = (double)((int)rpk->ReadUint32());
			break;
		}
		case L_INT64:{
			n = (double)((long long)rpk->ReadUint64());
			break;
		}
		default:{
			assert(0);
			break;
		}
	}
	lua_pushnumber(L,n);
}

static inline void un_pack_string(net::StreamRPacket *rpk,lua_State *L){
	size_t len;
	const char *data = (const char*)rpk->ReadBin(len);
	lua_pushlstring(L,data,(size_t)len);
}


static int un_pack_table(net::StreamRPacket *rpk,lua_State *L){
	int size = rpk->ReadUint32();
	int i = 0;
	lua_newtable(L);
	for(; i < size; ++i){
		int key_type,value_type;
		key_type = rpk->ReadUint8();
		if(key_type == L_STRING){
			un_pack_string(rpk,L);
		}else if(key_type >= L_FLOAT && key_type <= L_INT64){
			un_pack_number(rpk,L,key_type);
		}else
			return -1;
		value_type = rpk->ReadUint8();
		if(value_type == L_STRING){
			un_pack_string(rpk,L);
		}else if(value_type >= L_FLOAT && value_type <= L_INT64){
			un_pack_number(rpk,L,value_type);
		}else if(value_type == L_BOOL){
			un_pack_boolean(rpk,L);
		}else if(value_type == L_TABLE){
			if(0 != un_pack_table(rpk,L)){
				return -1;
			}
		}else
			return -1;
		lua_rawset(L,-3);
	}
	return 0;
}

inline static lua_packet_t lua_getluapacket(lua_State *L, int index) {
	return (lua_packet_t)lua_touserdata(L,index);//luaL_checkudata(L, index, LUARPACKET_METATABLE);
}


static int ReadUint8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushinteger(L,(lua_Integer)rpk->ReadUint8());
	return 1;
}

static int ReadUint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushinteger(L,(lua_Integer)rpk->ReadUint16());
	return 1;
}

static int ReadUint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushinteger(L,(lua_Integer)rpk->ReadUint32());	
	return 1;
}

static int ReadInt8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushinteger(L, (lua_Integer)rpk->ReadInt8());
	return 1;
}

static int ReadInt16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushinteger(L, (lua_Integer)rpk->ReadInt16());
	return 1;
}

static int ReadInt32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushinteger(L, (lua_Integer)rpk->ReadInt32());
	return 1;
}


static int ReadDouble(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	lua_pushnumber(L,(lua_Number)rpk->ReadDouble());	
	return 1;
}

static int ReadString(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	size_t len;
	const char* str = (const char*)rpk->ReadBin(len);//rpk->ReadString();
	if(str)
		lua_pushlstring(L,str,len);
	else
		lua_pushnil(L);
	return 1;
}

static int ReadTable(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::StreamRPacket *rpk = dynamic_cast<net::StreamRPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	int type = rpk->ReadUint8();
	if(type != L_TABLE){
		lua_pushnil(L);
		return 1;
	}
	int old_top = lua_gettop(L);
	int ret = un_pack_table(rpk,L);
	if(0 != ret){
		lua_settop(L,old_top);
		lua_pushnil(L);
	}
	return 1;
}

static int WriteUint8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");	
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	unsigned char value = (unsigned char)lua_tointeger(L, 2);
	wpk->WriteUint8(value);
	return 0;
}

static int WriteUint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");	
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	unsigned short value = (unsigned short)lua_tointeger(L, 2);
	wpk->WriteUint16(value);
	return 0;
}

static int WriteUint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");	
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	unsigned int value = (unsigned int)lua_tointeger(L, 2);
	wpk->WriteUint32(value);
	return 0;
}

static int WriteDouble(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");	
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	double value = (double)lua_tonumber(L, 2);
	wpk->WriteDouble(value);
	return 0;
}

static int WriteString(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");	
	if(lua_type(L,2) != LUA_TSTRING)
		return luaL_error(L,"invaild arg2");
	size_t len;
	const char *val = lua_tolstring(L, 2,&len);
	wpk->WriteBin((void*)val,len);
	return 0;
}

static int WriteTable(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");	
	if(LUA_TTABLE != lua_type(L, 2))
		return luaL_error(L,"argument should be lua table");
	if(0 != luabin_pack_table(wpk,L,-1))
		return luaL_error(L,"table should not hava metatable");	
	return 0;	
}

static int RewriteUint8(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");
	net::write_pos wpos = (net::write_pos)lua_tointeger(L,2);
	unsigned char value = (unsigned char)lua_tointeger(L, 3);
	wpk->RewriteUint8(wpos, value);
	return 0;
}

static int RewriteUint16(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");
	net::write_pos wpos = (net::write_pos)lua_tointeger(L,2);
	unsigned short value = (unsigned short)lua_tointeger(L, 3);
	wpk->RewriteUint16(wpos, value);
	return 0;
}

static int RewriteUint32(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");
	net::write_pos wpos = (net::write_pos)lua_tointeger(L,2);
	unsigned int value = (unsigned int)lua_tointeger(L, 3);
	wpk->RewriteUint32(wpos, value);
	return 0;
}

static int RewriteDouble(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");
	net::write_pos wpos = (net::write_pos)lua_tointeger(L,2);
	double value = (double)lua_tonumber(L, 3);
	wpk->RewriteDouble(wpos, value);
	return 0;
}

static int GetWritePos(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet)return luaL_error(L,"invaild opration");
	net::StreamWPacket *wpk = dynamic_cast<net::StreamWPacket*>(p->packet);
	if(!wpk)return luaL_error(L,"invaild opration");
	lua_pushinteger(L, (lua_Integer)wpk->GetWritePos());
	return 1;
}

static int NewWPacket(lua_State *L){
	int argtype = lua_type(L,1); 
	if(argtype == LUA_TNUMBER || argtype == LUA_TNIL || argtype == LUA_TNONE){
		size_t len = 0;
		if(argtype == LUA_TNUMBER) {
			len = lua_tointeger(L,1);
		}
		if(len < 64) {
			len = 64;
		}
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
		luaL_getmetatable(L, LUAWPACKET_METATABLE);
		lua_setmetatable(L, -2);
		p->packet = new net::WPacket(len);
		return 1;
	} else if(argtype ==  LUA_TUSERDATA) {
		lua_packet_t o = lua_getluapacket(L,1);
		if(!o || !o->packet || o->packet->Type() != RPACKET) {
			return luaL_error(L,"invaild opration for arg1");
		}
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
		luaL_getmetatable(L, LUAWPACKET_METATABLE);
		lua_setmetatable(L, -2);
		p->packet = new net::WPacket(*dynamic_cast<net::RPacket*>(o->packet));
		return 1;
	} else if(argtype == LUA_TTABLE) {
		net::WPacket* wpk = new net::WPacket(512);
		if(0 != luabin_pack_table(wpk,L,-1)){
			delete wpk;
			return luaL_error(L,"table should not hava metatable");	
		}else{
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
			luaL_getmetatable(L, LUAWPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->packet = wpk;
		}
		return 1;
	} else {
		return luaL_error(L,"invaild opration for arg1");
	}
}

static int NewRPacket(lua_State *L){
	if (lua_type(L,1) == LUA_TUSERDATA) {
		lua_packet_t o = lua_getluapacket(L,1);
		if(!o || !o->packet || o->packet->Type() != RPACKET) {
			return luaL_error(L,"invaild opration for arg1");
		}
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
		luaL_getmetatable(L, LUARPACKET_METATABLE);
		lua_setmetatable(L, -2);
		p->packet = new net::RPacket(*dynamic_cast<net::RPacket*>(o->packet));
		return 1;
	} else {
		return luaL_error(L,"invaild opration for arg1");
	}
}

static int NewRawPacket(lua_State *L){
	int argtype = lua_type(L,1);
	if(argtype == LUA_TSTRING){
		const char *str;
		size_t len;
		str = lua_tolstring(L,1,&len);
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
		luaL_getmetatable(L, LUARAWPACKET_METATABLE);
		lua_setmetatable(L, -2);
		p->packet = new net::RawBinPacket(str,len);
		return 1;		
	}else if(argtype == LUA_TUSERDATA){
		lua_packet_t o = lua_getluapacket(L,1);
		if(!o || !o->packet || o->packet->Type() != RAWBINARY) {
			return luaL_error(L,"invaild opration for arg1");
		}
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
		luaL_getmetatable(L, LUARAWPACKET_METATABLE);
		lua_setmetatable(L, -2);
		p->packet = o->packet->Clone();
		return 1;
	}else{
		return luaL_error(L,"invaild opration for arg1");
	}	
}


static int ReadBinary(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::RawBinPacket *rpk = dynamic_cast<net::RawBinPacket*>(p->packet);
	if(!rpk) return luaL_error(L,"invaild opration");
	size_t len;
	const char* str = (const char*)rpk->ReadBin(len);
	if(str)
		lua_pushlstring(L,str,len);
	else
		lua_pushnil(L);
	return 1;
}

static int destroy_luapacket(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->packet){
		delete p->packet;
	}
    return 0;
}

void push_luaPacket(lua_State *L,net::Packet *rpk){
	lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
	switch(rpk->Type()){
		case WPACKET:luaL_getmetatable(L, LUAWPACKET_METATABLE);break;
		case RPACKET:luaL_getmetatable(L, LUARPACKET_METATABLE);break;
		case HTTPPACKET:luaL_getmetatable(L, LUAHTTPPACKET_METATABLE);break;
		case RAWBINARY:luaL_getmetatable(L, LUARAWPACKET_METATABLE);break;
		default:{
			assert(0);
			lua_pushnil(L);
			return;
		}
	}
	lua_setmetatable(L, -2);
	p->packet = rpk->Clone();	
}

net::Packet *toLuaPacket(lua_State *L,int index){
	lua_packet_t p = lua_getluapacket(L,index);
	if(p) return p->packet;
	return NULL;
}

//httppacket

static int GetUrl(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::HttpPacket *rpk = dynamic_cast<net::HttpPacket*>(p->packet);
	const char *url = rpk->GetUrl();
	if(url)
		lua_pushstring(L,url);
	else
		lua_pushnil(L);
	return 1;	
}

static int GetStatus(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::HttpPacket *rpk = dynamic_cast<net::HttpPacket*>(p->packet);
	const char *status = rpk->GetStatus();
	if(status)
		lua_pushstring(L,status);
	else
		lua_pushnil(L);
	return 1;	
}

static int GetBody(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::HttpPacket *rpk = dynamic_cast<net::HttpPacket*>(p->packet);
	size_t len;
	const char *body = rpk->GetBody(len);
	if(body && len > 0)
		lua_pushlstring(L,body,len);
	else
		lua_pushnil(L);
	return 1;	
}

#include "http-parser/http_parser.h"

const char *http_method_name[] = 
  {
#define XX(num, name, string) #string,
  HTTP_METHOD_MAP(XX)
#undef XX
  };

static int GetHeaders(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::HttpPacket *rpk = dynamic_cast<net::HttpPacket*>(p->packet);
	rpk->PushHeaders(L);
	return 1;	
}

static int GetMethod(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if (!p || !p->packet) return luaL_error(L,"invaild opration");
	net::HttpPacket *rpk = dynamic_cast<net::HttpPacket*>(p->packet);
	int method = rpk->GetMethod();
	if(method < 0)
		lua_pushnil(L);
	else{
		lua_pushstring(L,http_method_name[method]);
	}	
	return 1;
}

#define SET_FUNCTION(L,NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

void RegLuaPacket(lua_State *L) {

    luaL_Reg packet_mt[] = {
        {"__gc", destroy_luapacket},
        {NULL, NULL}
    };

    luaL_Reg rpacket_methods[] = {
        {"ReadU8",  ReadUint8},
        {"ReadU16", ReadUint16},
        {"ReadU32", ReadUint32},
        {"ReadI8",  ReadInt8},
        {"ReadI16", ReadInt16},
        {"ReadI32", ReadInt32},        
        {"ReadNum", ReadDouble},        
        {"ReadStr", ReadString},
        {"ReadTable", ReadTable},
        {NULL, NULL}
    };

    luaL_Reg wpacket_methods[] = {                 
        {"WriteU8", WriteUint8},
        {"WriteU16",WriteUint16},
        {"WriteU32",WriteUint32},
        {"WriteNum",WriteDouble},        
        {"WriteStr",WriteString},
        {"WriteTable",WriteTable},
        {"RewriteU8",RewriteUint8},
        {"RewriteU16",RewriteUint16},
        {"RewriteU32",RewriteUint32},
        {"RewriteNum",RewriteDouble},
        {"GetWritePos",GetWritePos},
        {NULL, NULL}
    }; 

    luaL_Reg rawpacket_methods[] = {                 
		{"ReadBinary", ReadBinary},
        {NULL, NULL}
    };

    luaL_Reg httppacket_methods[] = {                 
        {"GetUrl", GetUrl},
        {"GetStatus",GetStatus},
        {"GetBody",GetBody},       
        {"GetHeaders",GetHeaders},
        {"GetMethod",GetMethod},
        {NULL, NULL}
    };


    luaL_newmetatable(L, LUARPACKET_METATABLE);
    luaL_setfuncs(L, packet_mt, 0);

    luaL_newlib(L, rpacket_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, LUAWPACKET_METATABLE);
    luaL_setfuncs(L, packet_mt, 0);

    luaL_newlib(L, wpacket_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, LUARAWPACKET_METATABLE);
    luaL_setfuncs(L, packet_mt, 0);

    luaL_newlib(L, rawpacket_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

                     
    luaL_newmetatable(L, LUAHTTPPACKET_METATABLE);
    luaL_setfuncs(L, packet_mt, 0);

    luaL_newlib(L, httppacket_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1); 

    SET_FUNCTION(L,"NewWPacket",NewWPacket);
    SET_FUNCTION(L,"NewRPacket",NewRPacket);
    SET_FUNCTION(L,"NewRawPacket",NewRawPacket);

}