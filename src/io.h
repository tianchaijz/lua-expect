#ifndef IO_H
#define IO_H

#include <stdlib.h>

#include "timeout.h"

#ifdef _WIN32
#include "io_win.h"
#else
#include "io_unix.h"
#endif

/* IO error codes */
enum {
    IO_DONE = 0,     /* operation completed successfully */
    IO_TIMEOUT = -1, /* operation timed out */
    IO_CLOSED = -2,  /* the connection has been closed */
    IO_UNKNOWN = -3
};

#define IO_FD_INVALID (-1)

int io_waitfd(int *fd, int sw, timeout_t *tm);
int io_open(void);
int io_close(void);
void io_destroy(int *fd);
int io_select(int n, fd_set *rfds, fd_set *wfds, fd_set *efds, timeout_t *tm);
int io_write(int *fd, const char *data, size_t count, size_t *sent,
             timeout_t *tm);
int io_read(int *fd, char *data, size_t count, size_t *got, timeout_t *tm);
int io_setblocking(int *fd);
int io_setnonblocking(int *fd);
void io_sleep(double n);
const char *io_strerror(int err);

#endif /* IO_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
