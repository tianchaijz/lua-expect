#include "lpty.h"

#define LPTY_VERSION "0.0.1"

static int lpty_execvpe(const char *file, char **argv, char **envp);
static int lpty_login_tty(int slave_fd);

static int lpty_fork(int master, int slave, int *amaster, char *name,
                     struct termios *termp, struct winsize *winp);
static int lpty_spawn(lua_State *L);

LUALIB_API int luaopen_lpty(lua_State *L);

static int lpty_execvpe(const char *file, char **argv, char **envp) {
    char **old;
    int rc;

    old = environ;
    environ = envp;
    rc = execvp(file, argv);
    environ = old;

    return rc;
}

static int lpty_login_tty(int slave_fd) {
    int i;

    /* Create a new session. */
    setsid();
    for (i = 0; i < 3; i++)
        if (i != slave_fd)
            close(i);
#ifdef TIOCSCTTY
    if (ioctl(slave_fd, TIOCSCTTY, NULL) < 0) {
        perror("ioctl: ");
        return -1;
    }
#else
    {
        char *slave_name;
        int dummy_fd;

        slave_name = ttyname(slave_fd);
        if (slave_name == NULL)
            return -1;
        dummy_fd = open(slave_name, O_RDWR);
        if (dummy_fd < 0)
            return -1;
        close(dummy_fd);
    }
#endif

    /* Assign fd to the standard input, standard output, and standard error of
       the current process. */
    for (i = 0; i < 3; i++)
        if (slave_fd != i)
            if (dup2(slave_fd, i) < 0)
                return -1;
    if (slave_fd >= 3)
        close(slave_fd);

    return 0;
}

static int lpty_fork(int master, int slave, int *amaster, char *name,
                     struct termios *termp, struct winsize *winp) {
    int pid;

    switch (pid = fork()) {
    case -1:
        close(master);
        close(slave);
        return -1;

    case 0:
        /* Child. */
        close(master);
        if (lpty_login_tty(slave)) {
            perror("login_tty failed");
            _exit(1);
        }
        return 0;
    default:
        /* Parent. */
        *amaster = master;
        close(slave);
        return pid;
    }
}

static int lpty_spawn(lua_State *L) {
    int top;
    int i;
    int argc;
    int envc;
    char **argv;
    char **env;
    const char *file;
    char *cwd;
    struct winsize winp;
    int master;
    int slave;
    pid_t pid;
    char name[64];

    top = lua_gettop(L);
    if (top != 8 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
        !lua_isstring(L, 3) || !lua_istable(L, 4) || !lua_istable(L, 5) ||
        !lua_isstring(L, 6) || !lua_isnumber(L, 7) || !lua_isnumber(L, 8)) {
        return luaL_error(
            L, "spawn(master, slave, file, args, env, cwd, cols, rows)");
    }

    argc = luaL_getn(L, 4);
    argv = calloc(argc + 2, sizeof(char *));
    file = lua_tostring(L, 3);

    i = 0;
    argv[i++] = strdup(file);
    argv[argc + 1] = NULL;
    for (; i < argc + 1; i++) {
        lua_rawgeti(L, 4, i);
        argv[i] = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    i = 0;
    envc = luaL_getn(L, 5);
    env = calloc(envc + 1, sizeof(char *));
    env[envc] = NULL;
    for (; i < envc; i++) {
        lua_rawgeti(L, 5, i + 1);
        env[i] = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    cwd = strdup(lua_tostring(L, 6));

    winp.ws_xpixel = 0;
    winp.ws_ypixel = 0;
    winp.ws_col = lua_tointeger(L, 7);
    winp.ws_row = lua_tointeger(L, 8);

    master = lua_tointeger(L, 1);
    slave = lua_tointeger(L, 2);
    pid = lpty_fork(master, slave, &master, name, NULL, &winp);

    if (pid) {
        for (i = 0; i < argc + 2; i++)
            free(argv[i]);
        free(argv);
        for (i = 0; i < envc; i++)
            free(env[i]);
        free(env);
        free(cwd);
    }

    switch (pid) {
    case -1:
        lua_pushnil(L);
        lua_pushstring(L, "forkpty failed");
        return 2;
    case 0:
        if (strlen(cwd))
            chdir(cwd);

        if (setgid(getgid()) == -1) {
            perror("setgid failed");
            _exit(1);
        }
        if (setuid(getuid()) == -1) {
            perror("setuid failed");
            _exit(1);
        }

        lpty_execvpe(argv[0], argv, env);

        perror("execvp failed");
        _exit(1);
    }

    lua_pushboolean(L, 1);

    return 1;
}

static int lpty_open(lua_State *L) {
    int top;
    int rc;
    int master;
    int slave;
    char name[64];
    struct winsize winp;

    top = lua_gettop(L);

    if (top != 2 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
        return luaL_error(L, "open(cols: int, rows: int)");

    winp.ws_xpixel = 0;
    winp.ws_ypixel = 0;
    winp.ws_col = lua_tointeger(L, 1);
    winp.ws_row = lua_tointeger(L, 2);

    rc = openpty(&master, &slave, name, NULL, &winp);

    if (rc == -1) {
        lua_pushnil(L);
        lua_pushstring(L, "openpty failed");
        return 2;
    }

    lua_createtable(L, 0 /* narr */, 3 /* nrec */);

    lua_pushinteger(L, master);
    lua_setfield(L, -2, "master");

    lua_pushinteger(L, slave);
    lua_setfield(L, -2, "slave");

    lua_pushstring(L, name);
    lua_setfield(L, -2, "name");

    return 1;
}

static const struct luaL_Reg lpty_funcs[] = {
    {"open", lpty_open}, {"spawn", lpty_spawn}, {NULL, NULL}};

int luaopen_lpty(lua_State *L) {
    luaL_register(L, "lpty", lpty_funcs);

    return 1;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
