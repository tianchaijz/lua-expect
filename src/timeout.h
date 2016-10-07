#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <float.h>
#include <limits.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/* timeout control structure */
typedef struct timeout_s {
    double block; /* maximum time for blocking calls */
    double total; /* total number of miliseconds for operation */
    double start; /* time of start of operation */
} timeout_t;

/*=========================================================================*\
* Timeout management functions
\*=========================================================================*/
void timeout_init(timeout_t *tm, double block, double total);
void *timeout_markstart(timeout_t *tm);
double timeout_get(timeout_t *tm);
double timeout_getretry(timeout_t *tm);
double timeout_getstart(timeout_t *tm);
double timeout_gettime(void);

#define timeout_iszero(tm) ((tm)->block == 0.0)

#endif /* TIMEOUT_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
