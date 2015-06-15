
#ifndef _LUAUTIL_H
#define _LUAUTIL_H

extern "C"{
#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>
}
#include <string>

class luaRef{
public:
	luaRef(lua_State *L,int idx):L(L),rindex(-1){
		if(L){
			lua_pushvalue(L,idx);
			this->rindex = luaL_ref(L,LUA_REGISTRYINDEX);
			if(LUA_REFNIL == this->rindex)
				this->L = NULL;
			if(L) counter = new int(1);
		}
	}

	luaRef &operator = (const luaRef & other)
	{
		if(this != &other && other.L)
		{
			if(L && counter && --(*counter) <= 0)
			{
				luaL_unref(this->L,LUA_REGISTRYINDEX,this->rindex);
				delete counter;
			}
			counter = other.counter;
			++(*counter);
			this->rindex = other.rindex;
			this->L = other.L;	
		}
		return *this;
	}

	luaRef(const luaRef &other)
		:L(other.L),rindex(other.rindex),counter(other.counter)
	{
		if(L && counter) ++(*counter);
	}

	~luaRef(){
		if(L && counter){
			if(!(--(*counter)))
			{
				luaL_unref(L,LUA_REGISTRYINDEX,rindex);
				delete counter;
			}
		}
	}

	lua_State *GetLState(){
		return L;
	}
	
	int GetIndex(){
		return rindex;
	}

private:
	lua_State     *L;
	int 		   rindex;
	int*           counter;
};

#endif