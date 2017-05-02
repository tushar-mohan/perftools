/* $Id: papiex.h 55275 2008-06-02 20:58:48Z tmohan $ */

#ifndef PAPIEX_H
#define PAPIEX_H

#define PAPIEX_MAX_CALIPERS 32

#ifndef PAPIEX_MAX_IO_DESC
#  define PAPIEX_MAX_IO_DESC 1024
#endif

#define PAPIEX_DEFAULT_MPX_HZ 10 /* default multiplexing interval in Hertz */
#define PAPIEX_MPX_MIN_SAMPLES 10 /* if we get less samples than this, we complain */

#if defined(NO_PAPIEX)
#define PAPIEX_START
#define PAPIEX_STOP
#define PAPIEX_START_ARG(a,b)
#define PAPIEX_STOP_ARG(a)
#elif defined(_LANGUAGE_FORTRAN) || defined(__EFC) || (!defined(_LANGUAGE_C) && !defined(__GNUC__) && !defined(__ECC)) 
#define PAPIEX_START() call papiex_start(1,"")
#define PAPIEX_STOP() call papiex_stop(1)
#define PAPIEX_ACCUM() call papiex_accum(1)
#define PAPIEX_START_ARG(a,b) call papiex_start(a,b)
#define PAPIEX_ACCUM_ARG(a) call papiex_accum(a)
#define PAPIEX_STOP_ARG(a) call papiex_stop(a)
#else
#ifdef __cplusplus
extern "C" {
#endif
extern void papiex_start(int, char *);
extern void papiex_accum(int);
extern void papiex_stop(int);
#ifdef __cplusplus
}
#endif
#define PAPIEX_START() papiex_start(1,"")
#define PAPIEX_ACCUM() papiex_accum(1)
#define PAPIEX_STOP() papiex_stop(1)
#define PAPIEX_START_ARG(a,b) papiex_start(a,b)
#define PAPIEX_ACCUM_ARG(a) papiex_accum(a)
#define PAPIEX_STOP_ARG(a) papiex_stop(a)
#endif

#endif
