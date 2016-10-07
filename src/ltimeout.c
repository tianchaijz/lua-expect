#include "ltimeout.h"

/*=========================================================================*\
* Function prototypes
\*=========================================================================*/
static int ltimeout_gettime(lua_State *L);

static luaL_Reg ltimeout_funcs[] = {{"gettime", ltimeout_gettime},
                                    {NULL, NULL}};

LUALIB_API int luaopen_ltimeout(lua_State *L) {
    luaL_register(L, "ltimeout", ltimeout_funcs);
    return 0;
}

/*-------------------------------------------------------------------------*\
* Returns the time the system has been up, in secconds.
\*-------------------------------------------------------------------------*/
static int ltimeout_gettime(lua_State *L) {
    lua_pushnumber(L, timeout_gettime());
    return 1;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
