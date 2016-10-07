#include "io.h"

/*-------------------------------------------------------------------------*\
* I/O error strings
\*-------------------------------------------------------------------------*/
const char *io_strerror(int err) {
    switch (err) {
    case IO_DONE:
        return NULL;
    case IO_CLOSED:
        return "closed";
    case IO_TIMEOUT:
        return "timeout";
    default:
        perror("unknown");
        return "unknown error";
    }
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
