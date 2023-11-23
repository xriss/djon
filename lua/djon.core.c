
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define DJON_C 1
#include "djon.h"




/*

We can use this string as a string identifier or its address as a light 
userdata identifier. Both are unique values.

*/
const char *lua_djon_ptr_name="djon*ptr";


/*

check that a userdata at the given index is a djon object
return the djon_state ** if it does, otherwise return 0

*/
static djon_state ** lua_djon_get_ptr (lua_State *l, int idx)
{
djon_state **ptrptr=0;

	ptrptr = lua_touserdata(l, idx);

	if(ptrptr)
	{
		if( lua_getmetatable(l, idx) )
		{
			luaL_getmetatable(l, lua_djon_ptr_name);
			if( !lua_rawequal(l, -1, -2) )
			{
				ptrptr = 0;
			}
			lua_pop(l, 2);
			return ptrptr;
		}
	}

	return ptrptr;
}


/*

call lua_djon_get_ptr and raise an error on null ptr or *ptr then return *ptr

*/
static djon_state * lua_djon_check_ptr (lua_State *l, int idx)
{
djon_state **ptrptr=lua_djon_get_ptr(l,idx);

	if(ptrptr == 0)
	{
		luaL_error(l, "not djon userdata" );
	}

	if(*ptrptr == 0)
	{
		luaL_error(l, "null djon userdata" );
	}

	return *ptrptr;
}

/*

alloc a ptr

*/
static djon_state ** lua_djon_alloc_ptr(lua_State *l)
{
djon_state **ptrptr;
	ptrptr = (djon_state **)lua_newuserdata(l, sizeof(djon_state *));
	(*ptrptr)=0;
	luaL_getmetatable(l, lua_djon_ptr_name);
	lua_setmetatable(l, -2);
	return ptrptr;
}

/*

free pointer at given index

*/
static int lua_djon_free_ptr (lua_State *l, int idx)
{
djon_state **ptrptr=lua_djon_get_ptr(l,idx);

	if(ptrptr)
	{
		if(*ptrptr)
		{
			djon_clean(*ptrptr);
		}
		(*ptrptr)=0;
	}
	return 0;
}




/*

allocate and setup a djon state

*/
static int lua_djon_setup (lua_State *l)
{
djon_state **ptrptr;
const char *s;
const char *opts=0;

	ptrptr=lua_djon_alloc_ptr(l);

	(*ptrptr)=djon_setup();

	return 1;
}

/*

clean and free a djon state

*/
static int lua_djon_clean (lua_State *l)
{
	lua_djon_free_ptr(l, 1);
	return 0;
}





static int lua_djon_load (lua_State *l)
{
djon_state *ds=lua_djon_check_ptr(l,1);

	return 0;
}


static int lua_djon_save (lua_State *l)
{
djon_state *ds=lua_djon_check_ptr(l,1);

	return 0;
}


LUALIB_API int luaopen_djon_core (lua_State *l)
{
	const luaL_Reg lib[] =
	{
		{"setup",					lua_djon_setup},
		{"clean",					lua_djon_clean},
		{"load",					lua_djon_load},
		{"save",					lua_djon_save},
		{0,0}
	};

	const luaL_Reg meta[] =
	{
		{"__gc",			lua_djon_clean},
		{0,0}
	};

	luaL_newmetatable(l, lua_djon_ptr_name);
	luaL_setfuncs(l, meta, 0);
	lua_pop(l,1);

	lua_newtable(l);
	luaL_setfuncs(l, lib, 0);

	return 1;
}

