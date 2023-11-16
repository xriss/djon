
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define DJON_C 1
#include "djon.h"


static int lua_djon_load_file (lua_State *l)
{
	return 0;
}


static int lua_djon_save_file (lua_State *l)
{
	return 0;
}


LUALIB_API int luaopen_wetgenes_chipmunk_core (lua_State *l)
{
	const luaL_Reg lib[] =
	{
		{"load_file",					lua_djon_load_file},
		{"save_file",					lua_djon_save_file},
		{0,0}
	};
	lua_newtable(l);
	luaL_setfuncs(l, lib, 0);
	return 1;
}

