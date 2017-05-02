#ifndef PAPIEX_INTERNAL_H
#define PAPIEX_INTERNAL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <fenv.h>
#include <getopt.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <search.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_MONITOR
#include "monitor.h"
#endif

#include "papiex.h"

//#ifdef DEBUG
#define PAPIEX_DEBUG_THREADID (unsigned long)((_papiex_threaded ? PAPI_thread_id() : 0))
#define LIBPAPIEX_DEBUG(...) { if (_papiex_debug) { fprintf(stderr,"libpapiex debug: %lu,0x%lx,%s ",(unsigned long)getpid(),PAPIEX_DEBUG_THREADID,__func__); fprintf(stderr, __VA_ARGS__); } }
#define PAPIEX_DEBUG(...) { extern char *tool; if (debug) { fprintf(stderr,"%s debug: %s ",tool,__func__); fprintf(stderr, __VA_ARGS__); } }
//#else
//#define LIBPAPIEX_DEBUG(...) 
//#define PAPIEX_DEBUG(...) 
//#endif

#define LIBPAPIEX_WARN(...) { fprintf(stderr,"libpapiex warning: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }
#define LIBPAPIEX_ERROR(...) { fprintf(stderr,"libpapiex error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); monitor_real_exit(1); }
#define LIBPAPIEX_PAPI_ERROR(name,code) { LIBPAPIEX_ERROR("PAPI error from %s, %d, %s", name, code, PAPI_strerror(code)); }
#define LIBPAPIEX_PAPI_WARN(name,code) { LIBPAPIEX_WARN("PAPI error from %s, %d, %s", name, code, PAPI_strerror(code)); }

#define PAPIEX_WARN(...) { extern char *tool; fprintf(stderr,"%s warning: ",tool); fprintf(stderr, __VA_ARGS__); }
#define PAPIEX_ERROR(...) { extern char *tool; fprintf(stderr,"%s error: ",tool); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); exit(1); }
#define PAPIEX_PAPI_ERROR(name,code) { PAPIEX_ERROR("PAPI error from %s, %d, %s\n", name, code, PAPI_strerror(code)); }
#define PAPIEX_PAPI_WARN(name,code) { PAPIEX_WARN("PAPI error from %s, %d, %s\n", name, code, PAPI_strerror(code)); }
#define PAPIEX_PAPI_DEBUG(name,code) { PAPIEX_DEBUG("PAPI error from %s, %d, %s\n", name, code, PAPI_strerror(code)); }

#ifndef IO_PROFILE_LIB
#define IO_PROFILE_LIB "libpapiexio.so"
#endif

#ifndef THREADSYNC_PROFILE_LIB
#define THREADSYNC_PROFILE_LIB "libpapiexmpsync.so"
#endif

#ifndef MPI_PROFILE_LIB
#define MPI_PROFILE_LIB "libpapiexmpi.so"
#endif

#ifdef HAVE_PAPI
#include "papi.h"

#if (PAPI_VERSION_MAJOR(PAPI_VERSION) < 3)
#error "papiex only supports PAPI version 3 or greater."
#endif

#if (PAPI_VERSION_REVISION(PAPI_VERSION) < 3)
/* Old <= 3.0.2 definitions */
#ifndef PAPI_PRESET_MASK
#define PAPI_PRESET_MASK PRESET_MASK
#endif
#ifndef PAPI_NATIVE_MASK
#define PAPI_NATIVE_MASK NATIVE_MASK
#endif
#endif

#ifndef PAPI_TLS_USER_LEVEL1
#ifdef PAPI_USR1_TLS
#define PAPI_TLS_USER_LEVEL1 PAPI_USR1_TLS
#else
#error "No definition for PAPI_TLS_USER_LEVEL1 or PAPI_USR1_TLS!"
#endif
#endif

#define MIPS_VENDOR_STRING "MIPS"
#define MIPS_MALTA_STRING "25Kf"
#define SC_VENDOR_STRING "SiCortex"
#define INTEL_VENDOR_STRING "GenuineIntel"

#define SC_ICE9A_STRING "ICE9A"
#define SC_ICE9B_STRING "ICE9B"
#define INTEL_COREI7_STRING "Intel Core i7"
#define INTEL_ITANIUM2_MODEL_STRING "32"

static inline int is_intel_core_processor(int vendor, int family, int m) {
  if ((vendor==1) &&
      (family==6) &&
      ((m==26) || (m==30) || (m==37) || (m==44) || (m==46) || (m==47)))
     return m;
  return 0;
}
static inline int is_intel_i7(int vendor, int m) {
  if ((vendor == 1) &&
      ((m==21) || (m==26) || (m==30) || (m==37) || (m==44) || (m==46) || (m==47)))
    return m;
  return 0;
}

extern void _papiex_dump_event_info(FILE *out, int eventcode, int verbose);
extern void _papiex_dump_memory_info(FILE *out);

#endif

#define MPIP_ENV "MPIP"
#define PAPIEX_DEFAULT_ARGS "PAPIEX_DEFAULT_ARGS"
#define PAPIEX_ENV "PAPIEX_OPTIONS"
#define PAPIEX_OUTPUT_ENV "PAPIEX_OUTPUT"
#define PAPIEX_LDLP "LD_LIBRARY_PATH"
#define PAPIEX_INIT_THREADS 256
#define GPTL_OPT_ENV "GPTL_OPTIONS"
#define GPTL_ON_ENV "GPTL_ON"
#define GPTL_OFF_ENV "GPTL_OFF"

#ifndef MAX_LABEL_SIZE
#define MAX_LABEL_SIZE 64
#endif

typedef struct {
  unsigned long long bytes;
} papiex_rdwr_t;

typedef struct {
  int flags;
  int mode;
  char *misc;
} papiex_open_t;

/* Note: SEEK_STRIDED MUST be defined as 0
 *       as we depend on the variables being
 *       memset to zero, and hence assume 
 *       a default STRIDED access pattern.
 */
#define SEEK_STRIDED 0
#define SEEK_SEQ     1
#define SEEK_RANDOM  2

typedef struct {
  unsigned long long num_calls;
  unsigned long long tot_abs_seek_bytes;
  unsigned long long tot_acc_bytes;
  unsigned long long stride; /* valid, if acc_type = SEEK_STRIDED */
  unsigned long long last_seek_pos;
  int acc_type ; /* SEEK_RANDOM, SEEK_STRIDED, SEEK_SEQ */
  int num_rewinds;
} papiex_seek_t;

typedef union {
  papiex_open_t open;
  papiex_rdwr_t rdwr;
  papiex_seek_t seek;
} papiex_specific_t;

/* For each read/write call, we aggregate statistics into bins
 * In each bin we keep the start time for the bin, the bytes
 * moved and the time taken.
 */
typedef struct {
  struct timeval bin_start_time; /* gettimeofday is used to set this */
  unsigned long long bytes;
  unsigned long long usecs;
  void *prev_bin_ptr;
  void *next_bin_ptr;
} call_bin_t;
  

typedef struct {
  unsigned long long num_calls;
  unsigned long long usecs;
  papiex_specific_t spec;
  call_bin_t *call_bin_ptr;
} papiex_percall_data_t;

#ifndef MAX_IO_CALL_TYPES
#define MAX_IO_CALL_TYPES 64
#endif

#define PAPIEX_MAX_COUNTERS 128

/* Cost entry for miss penalties */
typedef struct {
  const char* event;
  int min;
  int max;
  int avg;
} event_costs_t;

#define COST_STALLS_MIN 0
#define COST_STALLS_AVG 1
#define COST_STALLS_MAX 2

typedef struct {
  papiex_percall_data_t call_data[MAX_IO_CALL_TYPES];
} papiex_perdesc_data_t;

typedef struct {
  char label[MAX_LABEL_SIZE];
  float used;
  int depth;
#ifdef HAVE_PAPI
  long long tmp_real_usec;
  long long real_usec;
  long long tmp_virt_usec;
  long long virt_usec;
  long long tmp_real_cyc;
  long long real_cyc;
  long long virt_cyc;
  long long tmp_virt_cyc;
  long long mpiprof; /* in wall clock time, or cycles */
  long long tmp_mpiprof; /* in wall clock time, or cycles */
  long long mpisyncprof; /* in wall clock time, or cycles */
  long long tmp_mpisyncprof; /* in wall clock time, or cycles */
  long long ioprof;  /* in wall clock time, or cycles */
  long long tmp_ioprof;  /* in wall clock time, or cycles */
  long long threadsyncprof;  /* in wall clock time, or cycles */
  long long tmp_threadsyncprof;  /* in wall clock time, or cycles */
  long long tmp_counters[PAPIEX_MAX_COUNTERS];
  long long counters[PAPIEX_MAX_COUNTERS];
#endif
} papiex_caliper_data_t;

/* The structures below are used only for variance 
 * calculations where if double fields are not used,
 * overflows may occur 
 */
typedef struct {
  char label[MAX_LABEL_SIZE];
  float used;
  int depth;
#ifdef HAVE_PAPI
  double tmp_real_usec;
  double real_usec;
  double tmp_virt_usec;
  double virt_usec;
  double tmp_real_cyc;
  double real_cyc;
  double virt_cyc;
  double tmp_virt_cyc;
  double long mpiprof; /* in wall clock time, or cycles */
  double long mpisyncprof; /* in wall clock time, or cycles */
  double long ioprof;  /* in wall clock time, or cycles */
  double long threadsyncprof;  /* in wall clock time, or cycles */
  double tmp_counters[PAPIEX_MAX_COUNTERS];
  double counters[PAPIEX_MAX_COUNTERS];
#endif
} papiex_double_caliper_data_t;

typedef struct {
  unsigned tid;
  int eventset;
  time_t stamp;    /* start time */
  time_t finish;   /* stop time */
  long long papiex_mpiprof; /* in wall clock time, or cycles */
  long long papiex_mpisyncprof; /* in wall clock time, or cycles */
  long long papiex_ioprof;  /* in wall clock time, or cycles */
  long long papiex_threadsyncprof;  /* in wall clock time, or cycles */
  int max_caliper_entries; /* number of entries in allocated structure */
  int max_caliper_used; /* highest number of referenced entry */
  struct hsearch_data htab; /* hash table for address to point mapping for GCC auto-instrumentation */
  papiex_caliper_data_t data[PAPIEX_MAX_CALIPERS];
} papiex_perthread_data_t;

typedef struct {
  unsigned tid;
  int eventset;
  time_t stamp;    /* start time */
  time_t finish;   /* stop time */
  double long papiex_mpiprof; /* in wall clock time, or cycles */
  double long papiex_mpisyncprof; /* in wall clock time, or cycles */
  double long papiex_ioprof;  /* in wall clock time, or cycles */
  double long papiex_threadsyncprof;  /* in wall clock time, or cycles */
  papiex_double_caliper_data_t data[PAPIEX_MAX_CALIPERS];
} papiex_double_perthread_data_t;

extern int _papiex_debug;
extern int _papiex_threaded;

/* Prototypes for gcc function instrumentation via -finstrument functions. */

extern void papiex_gcc_gate(int i);
extern void papiex_gcc_thread_init(papiex_perthread_data_t *thread_data);
extern void papiex_gcc_thread_shutdown(papiex_perthread_data_t *thread_data);

/* these functions are safe drop-in replacements
 * for PAPI_get/set_thrspecific for the time being.
 * The tag argument is ignored for now, but may be
 * used in the future for scoping.
 */
extern int get_thread_data(int tag, void **ptr);
extern int set_thread_data(int tag, void *ptr);
#define PAPI_get_thr_specific(tag,ptr) get_thread_data(tag,ptr)
#define PAPI_set_thr_specific(tag,ptr) set_thread_data(tag,ptr)

/* Superfast inline ultohexstr() */
#define ULTOHEXSTR_SZ 34
static inline char *ultohexstr(val, buf)
     unsigned long val;
     char buf[ULTOHEXSTR_SZ];
{
  const int radix = 16;
  const int uppercase = 0;
  register char *p;
  register int c;

  if (radix > 36 || radix < 2)
    return 0;

  p = buf + ULTOHEXSTR_SZ*sizeof(char);
  *--p = '\0';

  do {
    c = val % radix;
    val /= radix;
    if (c > 9)
      *--p = (uppercase ? 'A' : 'a') - 10 + c;
    else
      *--p = '0' + c;
  }
  while (val);
  return p;
}

extern void pretty_print(FILE* output, const char* desc, double count, int type, int make_indented, int no_newline, const char* str);

static const int TYPE_FLOAT=0;
static const int TYPE_LONG=1;
static const int TYPE_STRING=2;

/* Pretty printing for floats */
static inline void pretty_printf(FILE* output, const char* desc, int make_indented, float count) {
  if (isnan(count) || (count < 0))
    count = 0.0;
  pretty_print(output, desc, (double) count, TYPE_FLOAT, make_indented, 0, NULL);
}

/* Pretty printing for longs */
static inline void pretty_printl(FILE* output, const char* desc, int make_indented, long long count) {
  pretty_print(output, desc, (double) count, TYPE_LONG, make_indented, 0, NULL);
}

/* Pretty printing for strings */
static inline void pretty_prints(FILE* output, const char* desc, int make_indented, const char *str) {
  pretty_print(output, desc, 0, TYPE_STRING, make_indented, 0, str);
}

#include "pathscale.h"
#ifdef HAVE_BINUTILS
#  include "pclookup.h"
#endif

#define THR_SCOPE 1
#define PROC_SCOPE 2
#define JOB_SCOPE 3

#endif
