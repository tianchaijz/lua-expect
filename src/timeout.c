/*=========================================================================*\
* Timeout management functions
\*=========================================================================*/
#include "timeout.h"

/* min and max macros */
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? x : y)
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? x : y)
#endif

/*=========================================================================*\
* Exported functions.
\*=========================================================================*/
void timeout_init(timeout_t *tm, double block, double total) {
    tm->block = block;
    tm->total = total;
}

void *timeout_markstart(timeout_t *tm) {
    tm->start = timeout_gettime();
    return tm;
}

/*-------------------------------------------------------------------------*\
* Determines how much time we have left for the next system call,
* if the previous call was successful
* Input
*   tm: timeout control structure
* Returns
*   the number of ms left or -1 if there is no time limit
\*-------------------------------------------------------------------------*/
double timeout_get(timeout_t *tm) {
    double t;

    if (tm->block < 0.0 && tm->total < 0.0) {
        return -1;
    } else if (tm->block < 0.0) {
        t = tm->total - timeout_gettime() + tm->start;
        return MAX(t, 0.0);
    } else if (tm->total < 0.0) {
        return tm->block;
    } else {
        t = tm->total - timeout_gettime() + tm->start;
        return MIN(tm->block, MAX(t, 0.0));
    }
}

double timeout_getstart(timeout_t *tm) {
    return tm->start;
}

/*-------------------------------------------------------------------------*\
* Determines how much time we have left for the next system call,
* if the previous call was a failure
* Input
*   tm: timeout control structure
* Returns
*   the number of ms left or -1 if there is no time limit
\*-------------------------------------------------------------------------*/
double timeout_getretry(timeout_t *tm) {
    double t;

    if (tm->block < 0.0 && tm->total < 0.0) {
        return -1;
    } else if (tm->block < 0.0) {
        t = tm->total - timeout_gettime() + tm->start;
        return MAX(t, 0.0);
    } else if (tm->total < 0.0) {
        t = tm->block - timeout_gettime() + tm->start;
        return MAX(t, 0.0);
    } else {
        t = tm->total - timeout_gettime() + tm->start;
        return MIN(tm->block, MAX(t, 0.0));
    }
}

/*-------------------------------------------------------------------------*\
* Gets time in s, relative to January 1, 1970 (UTC)
* Returns
*   time in s.
\*-------------------------------------------------------------------------*/
#ifdef _WIN32
double timeout_gettime(void) {
    FILETIME ft;
    double t;

    GetSystemTimeAsFileTime(&ft);
    /* Windows file time (time since January 1, 1601 (UTC)) */
    t = ft.dwLowDateTime / 1.0e7 + ft.dwHighDateTime * (4294967296.0 / 1.0e7);
    /* convert to Unix Epoch time (time since January 1, 1970 (UTC)) */
    return (t - 11644473600.0);
}
#else
double timeout_gettime(void) {
    struct timeval v;

    gettimeofday(&v, (struct timezone *)NULL);
    /* Unix Epoch time (time since January 1, 1970 (UTC)) */
    return v.tv_sec + v.tv_usec / 1.0e6;
}
#endif

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
