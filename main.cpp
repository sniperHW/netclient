#include "LuaUtil.h"

extern bool Reg2Lua(lua_State *L);

int main(int argc,char **argv){
	lua_State *L;
	if(argc < 2){
		printf("usage testlua luafile\n");
		return 0;
	}	
	L = luaL_newstate();
	luaL_openlibs(L);
	Reg2Lua(L);
	if (luaL_dofile(L,argv[1])) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}
	return 0;
}