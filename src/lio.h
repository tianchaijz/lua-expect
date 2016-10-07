#ifndef LIO_H
#define LIO_H

#include <string.h>

#include "lauxlib.h"
#include "lua.h"
#include "lua_compat.h"

#include "io.h"
#include "timeout.h"

LUALIB_API int luaopen_lio(lua_State *L);

#endif /* LIO_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
