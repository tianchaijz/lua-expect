#include "io.h"

/*-------------------------------------------------------------------------*\
* Sleep for n seconds.
\*-------------------------------------------------------------------------*/
int io_sleep(double t) {
    if (t < 0.0)
        t = 0.0;
    if (t < DBL_MAX / 1000.0)
        t *= 1000.0;
    if (t > INT_MAX)
        t = INT_MAX;
    Sleep((int)t);

    return 0;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
