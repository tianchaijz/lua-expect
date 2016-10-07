#ifndef LTIMEOUT_H
#define LTIMEOUT_H

#include "lauxlib.h"
#include "lua.h"
#include "lua_compat.h"

#include "timeout.h"

LUALIB_API int luaopen_ltimeout(lua_State *L);

#endif /* LTIMEOUT_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
