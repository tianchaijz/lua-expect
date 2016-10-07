#ifndef LPTY_H
#define LPTY_H

#include "lua_compat.h"
#include "pty_compat.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

LUALIB_API int luaopen_lpty(lua_State *L);

#endif /* LPTY_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
