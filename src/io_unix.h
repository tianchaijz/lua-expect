#ifndef IO_UNIX_H
#define IO_UNIX_H
/*=========================================================================*\
* IO compatibilization module for Unix
\*=========================================================================*/

#include <string.h>

/* error codes */
#include <errno.h>
/* close function */
#include <unistd.h>
/* fnctnl function and associated constants */
#include <fcntl.h>
/* select function */
#include <sys/select.h>
/* struct timeval */
#include <sys/time.h>
/* sigpipe handling */
#include <signal.h>

#endif /* IO_UNIX_H */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
