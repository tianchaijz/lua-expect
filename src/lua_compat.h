#ifndef LUA_COMPAT_H
#define LUA_COMPAT_H

#include "lauxlib.h"
#include "lua.h"

#if (LUA_VERSION_NUM >= 502)
#undef luaL_register
#define luaL_register(L, n, f)                                                 \
    {                                                                          \
        if ((n) == NULL)                                                       \
            luaL_setfuncs(L, f, 0);                                            \
        else                                                                   \
            luaL_newlib(L, f);                                                 \
    }
#endif

#endif /* LUA_COMPAT_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
