/*=========================================================================*\
* IO compatibilization module for Unix
*
* The code is now interrupt-safe.
* The penalty of calling select to avoid busy-wait is only paid when
* the I/O call fail in the first place.
\*=========================================================================*/

#include "io.h"

/*-------------------------------------------------------------------------*\
* Wait for readable/writable fd with timeout
\*-------------------------------------------------------------------------*/
#define WAITFD_R 1
#define WAITFD_W 2
#define WAITFD_C (WAITFD_R | WAITFD_W)

int io_waitfd(int *fd, int sw, timeout_t *tm) {
    struct timeval tv;
    struct timeval *tp;
    double t;
    fd_set rfds;
    fd_set wfds;
    fd_set *rp;
    fd_set *wp;
    int rc;

    if (*fd >= FD_SETSIZE)
        return EINVAL;
    if (timeout_iszero(tm))
        return IO_TIMEOUT; /* optimize timeout == 0 case */
    do {
        /* must set bits within loop, because select may have modifed them */
        rp = wp = NULL;
        if (sw & WAITFD_R) {
            FD_ZERO(&rfds);
            FD_SET(*fd, &rfds);
            rp = &rfds;
        }
        if (sw & WAITFD_W) {
            FD_ZERO(&wfds);
            FD_SET(*fd, &wfds);
            wp = &wfds;
        }
        t = timeout_getretry(tm);
        tp = NULL;
        if (t >= 0.0) {
            tv.tv_sec = (int)t;
            tv.tv_usec = (int)((t - tv.tv_sec) * 1.0e6);
            tp = &tv;
        }
        rc = select(*fd + 1, rp, wp, NULL, tp);
    } while (rc == -1 && errno == EINTR);
    if (rc == -1)
        return errno;
    if (rc == 0)
        return IO_TIMEOUT;
    if (sw == WAITFD_C && FD_ISSET(*fd, &rfds))
        return IO_CLOSED;

    return IO_DONE;
}

/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int io_open(void) {
    /* instals a handler to ignore sigpipe or it will crash us */
    signal(SIGPIPE, SIG_IGN);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Close module
\*-------------------------------------------------------------------------*/
int io_close(void) {
    return 1;
}

/*-------------------------------------------------------------------------*\
* Close and inutilize fd
\*-------------------------------------------------------------------------*/
void io_destroy(int *fd) {
    if (*fd != IO_FD_INVALID) {
        close(*fd);
        *fd = IO_FD_INVALID;
    }
}

/*-------------------------------------------------------------------------*\
* Select with timeout control
\*-------------------------------------------------------------------------*/
int io_select(int n, fd_set *rfds, fd_set *wfds, fd_set *efds, timeout_t *tm) {
    int rc;
    double t;
    struct timeval tv;

    do {
        t = timeout_getretry(tm);
        tv.tv_sec = (int)t;
        tv.tv_usec = (int)((t - tv.tv_sec) * 1.0e6);
        /* timeout = 0 means no wait */
        rc = select(n, rfds, wfds, efds, t >= 0.0 ? &tv : NULL);
    } while (rc < 0 && errno == EINTR);

    return rc;
}

/*-------------------------------------------------------------------------*\
* Write with timeout
\*-------------------------------------------------------------------------*/
int io_write(int *fd, const char *data, size_t count, size_t *sent,
             timeout_t *tm) {
    int err;
    long put;

    *sent = 0;
    /* avoid making system calls on closed fd */
    if (*fd == IO_FD_INVALID)
        return IO_CLOSED;
    /* loop until we send something or we give up on error */
    for (;;) {
        put = (long)write(*fd, data, count);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return IO_DONE;
        }
        err = errno;
        /* EPIPE means the connection was closed */
        if (err == EPIPE)
            return IO_CLOSED;
        /* EPROTOTYPE means the connection is being closed (on Yosemite!)*/
        if (err == EPROTOTYPE)
            continue;
        /* we call was interrupted, just try again */
        if (err == EINTR)
            continue;
        /* if failed fatal reason, report error */
        if (err != EAGAIN)
            return err;
        /* wait until we can send something or we timeout */
        if ((err = io_waitfd(fd, WAITFD_W, tm)) != IO_DONE)
            return err;
    }

    /* can't reach here */
    return IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Read with timeout
\*-------------------------------------------------------------------------*/
int io_read(int *fd, char *data, size_t count, size_t *got, timeout_t *tm) {
    int err;
    long taken;

    *got = 0;
    if (*fd == IO_FD_INVALID)
        return IO_CLOSED;
    for (;;) {
        taken = (long)read(*fd, data, count);
        if (taken > 0) {
            *got = taken;
            return IO_DONE;
        }
        err = errno;
        if (taken == 0)
            return IO_CLOSED;
        if (err == EINTR)
            continue;
        if (err != EAGAIN)
            return err;
        if ((err = io_waitfd(fd, WAITFD_R, tm)) != IO_DONE)
            return err;
    }

    return IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Put fd into blocking mode
\*-------------------------------------------------------------------------*/
int io_setblocking(int *fd) {
    int flags;

    flags = fcntl(*fd, F_GETFL, 0);
    if (flags == 1) {
        return -1;
    }
    flags &= (~(O_NONBLOCK));

    return fcntl(*fd, F_SETFL, flags);
}

/*-------------------------------------------------------------------------*\
* Put fd into non-blocking mode
\*-------------------------------------------------------------------------*/
int io_setnonblocking(int *fd) {
    int flags;

    flags = fcntl(*fd, F_GETFL, 0);
    if (flags == 1) {
        return -1;
    }

    return fcntl(*fd, F_SETFL, flags | O_NONBLOCK);
}

/*-------------------------------------------------------------------------*\
* Sleep for n seconds.
\*-------------------------------------------------------------------------*/
void io_sleep(double n) {
    struct timespec t;
    struct timespec r;

    if (n < 0.0)
        n = 0.0;
    if (n > INT_MAX)
        n = INT_MAX;
    t.tv_sec = (int)n;
    n -= t.tv_sec;
    t.tv_nsec = (int)(n * 1000000000);
    if (t.tv_nsec >= 1000000000)
        t.tv_nsec = 999999999;
    while (nanosleep(&t, &r) != 0) {
        t.tv_sec = r.tv_sec;
        t.tv_nsec = r.tv_nsec;
    }

    return;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
