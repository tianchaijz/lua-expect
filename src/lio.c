#include "lio.h"

static int getfd(lua_State *L);
static int dirty(lua_State *L);
static void collect_fd(lua_State *L, int tab, int itab, fd_set *set,
                       int *max_fd);
static int check_dirty(lua_State *L, int tab, int dtab, fd_set *set);
static void return_fd(lua_State *L, fd_set *set, int max_fd, int itab, int tab,
                      int start);
static void make_assoc(lua_State *L, int tab);

static int lio_read(lua_State *L);
static int lio_write(lua_State *L);
static int lio_select(lua_State *L);
static int lio_destroy(lua_State *L);
static int lio_setblocking(lua_State *L);
static int lio_setnonblocking(lua_State *L);
static int lio_sleep(lua_State *L);

static luaL_Reg lio_funcs[] = {{"read", lio_read},
                               {"write", lio_write},
                               {"destroy", lio_destroy},
                               {"select", lio_select},
                               {"setblocking", lio_setblocking},
                               {"setnonblocking", lio_setnonblocking},
                               {"sleep", lio_sleep},
                               {NULL, NULL}};

/*=========================================================================*\
* Exported functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
LUALIB_API int luaopen_lio(lua_State *L) {
    luaL_register(L, "lio", lio_funcs);
    return 0;
}

/*=========================================================================*\
* Global Lua functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Waits for a set of fds until a condition is met or timeout.
*
* Each object that can be passed to the select function has to export
* method getfd() which returns the descriptor to be passed to the
* underlying select function. Another method, dirty(), should return
* true if there is data ready for reading (required for buffered input).
\*-------------------------------------------------------------------------*/

static int lio_read(lua_State *L) {
    int top;
    int size;
    char *buf;
    size_t got;
    int fd;
    int rc;

    top = lua_gettop(L);

    if (top != 3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
        !lua_isnumber(L, 3)) {
        return luaL_error(L, "read(fd: int, size: int, timeout: int)");
    }

    fd = lua_tointeger(L, 1);
    if (fd == IO_FD_INVALID) {
        return luaL_error(L, "invalid fd");
    }

    size = lua_tointeger(L, 2);
    if (size <= 0) {
        return luaL_error(L, "invalid size");
    }

    buf = (char *)calloc(size, sizeof(char));

    timeout_t tm;
    timeout_init(&tm, 0, lua_tonumber(L, 3));

    rc = io_read(&fd, buf, size, &got, &tm);
    if (rc != IO_DONE) {
        lua_pushnil(L);
        lua_pushstring(L, io_strerror(rc));
        free(buf);

        return 2;
    }

    lua_pushlstring(L, buf, got);
    free(buf);

    return 1;
}

static int lio_write(lua_State *L) {
    int top;
    size_t size;
    size_t sent;
    const char *data;
    int fd;
    int rc;

    top = lua_gettop(L);
    if (top != 3 || !lua_isnumber(L, 1) || !lua_isstring(L, 2) ||
        !lua_isnumber(L, 3)) {
        return luaL_error(L, "write(fd: int, data: string, timeout: int)");
    }

    fd = lua_tointeger(L, 1);
    if (fd == IO_FD_INVALID) {
        return luaL_error(L, "invalid fd");
    }

    data = luaL_checklstring(L, 2, &size);

    if (size == 0) {
        return luaL_error(L, "zero size");
    }

    timeout_t tm;
    timeout_init(&tm, 0, lua_tonumber(L, 3));

    rc = io_write(&fd, data, size, &sent, &tm);
    if (rc != IO_DONE) {
        lua_pushnil(L);
        lua_pushstring(L, io_strerror(rc));

        return 2;
    }

    lua_pushnumber(L, sent);

    return 1;
}

static int lio_destroy(lua_State *L) {
    int top;
    int fd;

    top = lua_gettop(L);

    if (top != 1 || !lua_isnumber(L, 1)) {
        return luaL_error(L, "destroy(fd: int)");
    }

    fd = lua_tointeger(L, 1);
    io_destroy(&fd);

    lua_pushnumber(L, fd);

    return 1;
}

static int lio_select(lua_State *L) {
    int rtab;
    int wtab;
    int itab;

    fd_set rset;
    fd_set wset;

    int rc;
    int ndirty;
    int max_fd;
    double t;
    timeout_t tm;

    max_fd = IO_FD_INVALID;
    t = luaL_optnumber(L, 3, -1);

    FD_ZERO(&rset);
    FD_ZERO(&wset);

    lua_settop(L, 3);

    lua_newtable(L);
    itab = lua_gettop(L);

    lua_newtable(L);
    rtab = lua_gettop(L);

    lua_newtable(L);
    wtab = lua_gettop(L);

    collect_fd(L, 1, itab, &rset, &max_fd);
    collect_fd(L, 2, itab, &wset, &max_fd);

    ndirty = check_dirty(L, 1, rtab, &rset);
    t = ndirty > 0 ? 0.0 : t;

    timeout_init(&tm, t, -1);
    timeout_markstart(&tm);

    rc = io_select(max_fd + 1, &rset, &wset, NULL, &tm);
    if (rc > 0 || ndirty > 0) {
        return_fd(L, &rset, max_fd + 1, itab, rtab, ndirty);
        return_fd(L, &wset, max_fd + 1, itab, wtab, 0);
        make_assoc(L, rtab);
        make_assoc(L, wtab);
        return 2;
    } else if (rc == 0) {
        lua_pushstring(L, "timeout");
        return 3;
    } else {
        lua_pushstring(L, "select failed");
        return 3;
    }
}

static int lio_setblocking(lua_State *L) {
    int top;
    int fd;
    int rc;

    top = lua_gettop(L);
    if (top != 1 || !lua_isnumber(L, 1)) {
        return luaL_error(L, "setblocking(fd: int)");
    }

    fd = lua_tointeger(L, 1);
    rc = io_setblocking(&fd);

    if (rc == -1) {
        lua_pushnil(L);
        lua_pushstring(L, "setblocking failed");
        return 2;
    }

    lua_pushboolean(L, 1);

    return 1;
}

static int lio_setnonblocking(lua_State *L) {
    int top;
    int fd;
    int rc;

    top = lua_gettop(L);
    if (top != 1 || !lua_isnumber(L, 1)) {
        return luaL_error(L, "setnonblocking(fd: int)");
    }

    fd = lua_tointeger(L, 1);
    rc = io_setnonblocking(&fd);

    if (rc == -1) {
        lua_pushnil(L);
        lua_pushstring(L, "setnonblocking failed");
        return 2;
    }

    lua_pushboolean(L, 1);

    return 1;
}

static int lio_sleep(lua_State *L) {
    int top;

    top = lua_gettop(L);

    if (top != 1 || !lua_isnumber(L, 1)) {
        return luaL_error(L, "sleep(fd: double)");
    }

    io_sleep(lua_tonumber(L, 1));

    return 0;
}

/*=========================================================================*\
* Internal functions
\*=========================================================================*/
static int getfd(lua_State *L) {
    double numfd;
    int fd;

    fd = IO_FD_INVALID;

    lua_pushstring(L, "getfd");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        if (lua_isnumber(L, -1)) {
            numfd = lua_tonumber(L, -1);
            fd = (numfd >= 0.0) ? (int)numfd : IO_FD_INVALID;
        }
    }
    lua_pop(L, 1);
    return fd;
}

static int dirty(lua_State *L) {
    int is;

    is = 0;

    lua_pushstring(L, "dirty");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        is = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    return is;
}

static void collect_fd(lua_State *L, int tab, int itab, fd_set *set,
                       int *max_fd) {
    int i;
    int n;

    i = 1;
    n = 0;

    /* nil is the same as an empty table */
    if (lua_isnil(L, tab))
        return;
    /* otherwise we need it to be a table */
    luaL_checktype(L, tab, LUA_TTABLE);
    for (;;) {
        int fd;
        lua_pushnumber(L, i);
        lua_gettable(L, tab);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        /* getfd figures out if this is a fd */
        fd = getfd(L);
        if (fd != IO_FD_INVALID) {
/* make sure we don't overflow the fd_set */
#ifdef _WIN32
            if (n >= FD_SETSIZE)
                luaL_argerror(L, tab, "too many file descriptors");
#else
            if (fd >= FD_SETSIZE)
                luaL_argerror(L, tab, "descriptor too large for set size");
#endif
            FD_SET(fd, set);
            n++;
            /* keep track of the largest descriptor so far */
            if (*max_fd == IO_FD_INVALID || *max_fd < fd)
                *max_fd = fd;
            /* make sure we can map back from descriptor to the object */
            lua_pushnumber(L, (lua_Number)fd);
            lua_pushvalue(L, -2);
            lua_settable(L, itab);
        }
        lua_pop(L, 1);
        i = i + 1;
    }
}

static int check_dirty(lua_State *L, int tab, int dtab, fd_set *set) {
    int ndirty;
    int i;

    if (lua_isnil(L, tab))
        return 0;

    ndirty = 0;
    i = 1;

    for (;;) {
        int fd;
        lua_pushnumber(L, i);
        lua_gettable(L, tab);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        fd = getfd(L);
        if (fd != IO_FD_INVALID && dirty(L)) {
            lua_pushnumber(L, ++ndirty);
            lua_pushvalue(L, -2);
            lua_settable(L, dtab);
            FD_CLR(fd, set);
        }
        lua_pop(L, 1);
        i = i + 1;
    }

    return ndirty;
}

static void return_fd(lua_State *L, fd_set *set, int max_fd, int itab, int tab,
                      int start) {
    int fd;

    for (fd = 0; fd < max_fd; fd++) {
        if (FD_ISSET(fd, set)) {
            lua_pushnumber(L, ++start);
            lua_pushnumber(L, (lua_Number)fd);
            lua_gettable(L, itab);
            lua_settable(L, tab);
        }
    }
}

static void make_assoc(lua_State *L, int tab) {
    int i;
    int atab;

    i = 1;

    lua_newtable(L);
    atab = lua_gettop(L);
    for (;;) {
        lua_pushnumber(L, i);
        lua_gettable(L, tab);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }

        lua_pushnumber(L, i);
        lua_pushvalue(L, -2);
        lua_settable(L, atab);
        lua_pushnumber(L, i);
        lua_settable(L, atab);

        i = i + 1;
    }
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
