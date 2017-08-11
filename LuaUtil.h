
#ifndef _LUAUTIL_H
#define _LUAUTIL_H

extern "C"{
#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>
}

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM < 503
/*
** Adapted from Lua 5.2.0
*/
static inline void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
	luaL_checkstack(L, nup+1, "too many upvalues");
	for (; l->name != NULL; l++) {	/* fill the table with given functions */
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)	/* copy upvalues to the top */
			lua_pushvalue(L, -(nup + 1));
		lua_pushcclosure(L, l->func, nup);	/* closure with those upvalues */
		lua_settable(L, -(nup + 3));
	}
	lua_pop(L, nup);	/* remove upvalues */
}

#define luaL_newlibtable(L,l)												\
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define luaL_newlib(L,l)  													\
  (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

static inline void *luaL_testudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      luaL_getmetatable(L, tname);  /* get correct metatable */
      if (!lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}

#endif


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