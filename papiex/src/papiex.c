#include "papiex_internal.h"


#define STRINGIFY(X) #X
#define ICE9_SPEC_FILE "ice9"
#define NEHALEM_SPEC_FILE "nehalem"
#define MIPS74K_SPEC_FILE "mips74k"
#define DEFAULT_SPEC_FILE "papi"

/*
#define PRINT_EVENT_COUNT(event,idx, event_array) \
    idx = get_event_index(event, (const char**) eventnames, eventcnt); \
    if (idx != -1) \
      fprintf(output, "%16lld\n", event_array.counters[idx]); \
    else \
      fprintf(output, "Not Available\n");
*/

#define ADD_DERIVED_DESC(metric, text) \
  if (!not_first_pass) { \
    sprintf(desc, "%-30s: %s\n", metric, text); \
    strncat(derived_events_desc, desc, sizeof(derived_events_desc)-1); \
  }

#define GET_DLSYM(dl_handle,name,real_ptr,fptr_type) \
    if (real_ptr == NULL) { \
      char* err; \
      dlerror(); \
      real_ptr = (fptr_type)dlsym(dl_handle, name);	\
      if ((err = dlerror()) != NULL) { \
        LIBPAPIEX_ERROR("dlsym(dl_handle,%s) failed. %s", name, err);	\
        real_ptr = NULL; \
      } \
      if (real_ptr)  LIBPAPIEX_DEBUG("%s : %p\n", name, real_ptr); \
    }

static const char* SCOPE_STRING[] = {"THREAD_SCOPE", "PROCESS_SCOPE", "JOB_SCOPE"};
static const int PAD_LENGTH = 46;


#ifdef HAVE_PAPI
static volatile int eventcnt = 0;
static char *eventnames[PAPIEX_MAX_COUNTERS];
static int eventcodes[PAPIEX_MAX_COUNTERS];
static const PAPI_exe_info_t *exeinfo;
static const PAPI_hw_info_t *hwinfo;
#endif
static volatile int multiplex;

/* there is a bug in PAPI that crashes PAPI_read, if it's called too soon
 * after PAPI_start in a multiplex program. We workaround it, if the 
 * environment variable: PAPIEX_PAPI_MPX_ZERO_COUNTS_BUG is set */
static int papi_mpx_zero_counts_bug = 0;
static unsigned long long init_virt_cyc = 0;

static int papi_broken_stop =0 ;
static int mpx_interval= PAPIEX_DEFAULT_MPX_HZ; // in Hertz
static int no_derived_stats;
int _papiex_threaded;
/* To satisfy eventinfo.c used in libpapiex.so */
char *tool = "papiex";
char *build_date = __DATE__;
char *build_time = __TIME__;

static int nthreads = 1;
static char base_output_path[PATH_MAX];
static char proc_output_path[PATH_MAX]="";
static char thread_output_path[PATH_MAX]="";
static char *file_output;  // this is set for prefix and file output
static const char* path_container = ".papiex.output";
static char *output_file_name = NULL;
static int file_prefix;
static int domain;
static int rusage;
static int memory;
static char derived_events_desc[PATH_MAX*4] = "\nDerived event descriptions:\n";
static int quiet;
static int no_follow_fork;
static char *process_name = NULL;
static char process_args[PATH_MAX] = "";
static const char *fullname;
static const char *processor;
static double clockrate;
int _papiex_debug = 0;
static void *thread_id_handle;
static char hostname[PATH_MAX] = "";
static int no_write = 0;   /* no file output */
static int write_only = 0; /* no output to console */
static int no_summary_stats = 0; /* don't generate summary statistics */
static int no_mpi_gather = 0; /* gather stats using MPI (only for classic) */
static int gcc_prof = 0;
static int pathscale_prof = PATHSCALE_NONE;
volatile __thread papiex_perthread_data_t *thread_data = NULL;

#ifdef PROFILING_SUPPORT
  static int no_mpi_prof = 0; 
  static int no_io_prof = 0; 
  static int no_threadsync_prof = 1; 
#else
  static int no_mpi_prof = 1; 
  static int no_io_prof = 1; 
  static int no_threadsync_prof = 1;
#endif
static int no_scientific = 0;
static int papiex_nextgen = 0; /* legacy mode if unset */
static int csv_output = 0;

static void *dlproc_handle = NULL;
static char proc_stats_path[PATH_MAX];
static char user_path_prefix[PATH_MAX];
static PAPI_all_thr_spec_t all_threads;
static int all_threads_size = PAPIEX_INIT_THREADS;

/* instead of using the global proc_init_time, we should
 * ideally add fields for start and end time in the
 * PAPI_all_thread structure
 */
static struct timeval proc_init_time ;
static struct timeval proc_fini_time ;

/* Note: is_mpied is set ONLY when the MPI library is _used_ 
 * We do not set it merely on linking with the MPI library.
 * (since mpicc links with the library even if the program
 * doesn't use MPI calls in its run). We actually intercept 
 * the call to MPI_Finalize and set this variable there. 
 */
static int is_mpied = 0;  /* set once we intercept MPI_Init or MPI_Init_thread */
static int monitor_fini_mpi_callback_occurred = 0; /* did the callback occur? */
static int called_process_shutdown = 0; /* ensure shutdown is not attempted twice */

typedef void (*papiex_mpiprof_init_fptr_t) (void);
typedef void (*papiex_mpiprof_set_gate_fptr_t) (int);
typedef void (*papiex_ioprof_init_fptr_t) (void);
typedef void (*papiex_ioprof_set_gate_fptr_t) (int);
typedef void (*papiex_threadsyncprof_init_fptr_t) (void);
typedef void (*papiex_threadsyncprof_set_gate_fptr_t) (int);
typedef void (*papiex_gcc_thread_init_fptr_t) (void *);
typedef void (*papiex_gcc_thread_shutdown_fptr_t) (void *);
typedef void (*papiex_gcc_set_gate_fptr_t) (int);
typedef void (*papiex_pathscale_thread_init_fptr_t) (void *);
typedef void (*papiex_pathscale_thread_shutdown_fptr_t) (void *);
typedef void (*papiex_pathscale_set_gate_fptr_t) (int);
static papiex_mpiprof_init_fptr_t fptr_papiex_mpiprof_init = NULL;
static papiex_mpiprof_set_gate_fptr_t fptr_papiex_mpiprof_set_gate = NULL;
static papiex_ioprof_init_fptr_t fptr_papiex_ioprof_init = NULL;
static papiex_ioprof_set_gate_fptr_t fptr_papiex_ioprof_set_gate = NULL;
static papiex_threadsyncprof_init_fptr_t fptr_papiex_threadsyncprof_init = NULL;
static papiex_threadsyncprof_set_gate_fptr_t fptr_papiex_threadsyncprof_set_gate = NULL;
static papiex_gcc_thread_init_fptr_t fptr_papiex_gcc_thread_init = NULL;
static papiex_gcc_thread_shutdown_fptr_t fptr_papiex_gcc_thread_shutdown = NULL;
static papiex_gcc_set_gate_fptr_t fptr_papiex_gcc_set_gate = NULL;
static papiex_pathscale_thread_init_fptr_t fptr_papiex_pathscale_thread_init = NULL;
static papiex_pathscale_thread_shutdown_fptr_t fptr_papiex_pathscale_thread_shutdown = NULL;
static papiex_pathscale_set_gate_fptr_t fptr_papiex_pathscale_set_gate = NULL;

static char *stringify_domain(int domain);
static void print_thread_stats(FILE *output, papiex_perthread_data_t *thread, unsigned long tid, 
                               int scope, float process_walltime);

static void print_process_stats(PAPI_all_thr_spec_t *process, 
                                papiex_perthread_data_t *process_data_sums,
                                FILE *output);
//static void classic_print_global_stats(papiex_perthread_data_t *_task_data, FILE *output);

static const char *suffix = ".txt";

static int get_next_gen(const char *path);
static inline int get_event_index(const char * name, const char *eventnames[], int count);
static inline int get_num_tasks(void);
static void print_banner(FILE*);
static const char* spec_file = NULL;
static int user_specified_spec = 0; // did the user specify the spec file?
static int myrank = 0;
static int ntasks = 1;

/* get_thread_data will be a safe replacement to
   PAPI_get_thr_specific for the time being.
   The tag argument is ignored for now, but may be
   used for specifying a scope in the future */
int get_thread_data(int tag, void **data_ptr) {
  *data_ptr = (void *)thread_data;
  return PAPI_OK;
}

/* set_thread_data will be a safe replacement to
   PAPI_set_thr_specific for the time being.
   The tag argument is ignored for now, but may be
   used for specifying a scope in the future */
int set_thread_data(int tag, void *data_ptr) {
  thread_data = (papiex_perthread_data_t *)data_ptr;
  return PAPI_OK;
}

static int file_exists(const char * fileName) {
  struct stat buf;
  int i = stat (fileName, &buf);
  /* File found */
  if ( i == 0 ) return 1;
  return 0;
}

static float get_running_usecs(void) {
  unsigned long long cyc = PAPI_get_virt_cyc();
  float usecs = (float)(cyc - init_virt_cyc)/clockrate;
  LIBPAPIEX_DEBUG("Running time so far: %.3f\n", usecs);
  return usecs;
}


static void set_base_output_path(char* prefix, char* user_spec_file) {
  if (user_spec_file && strlen(user_spec_file)) {
    if (is_mpied) { 
      LIBPAPIEX_WARN("Output file cannot be specified for MPI programs. Ignoring it.\n");
    }
    else {
      /* If the user specified a path, that supersedes */
      LIBPAPIEX_DEBUG("User-specified output file: %s\n", user_spec_file);
      strncpy(base_output_path, user_spec_file, PATH_MAX);
      int gen = get_next_gen(user_spec_file);
      if (gen) {
        sprintf(base_output_path, "%s.%d", user_spec_file, gen);
        LIBPAPIEX_WARN("User-specified file %s already exists, naming file %s\n", user_spec_file, base_output_path);
      }
      return;
    }
  }

  int instance = 0;
  char *host = hostname;
  if (strchr(hostname, '.'))
    host = strtok(hostname, ".");

  if (!prefix) prefix = "";

  /* Figure out the instance number */
  do {
    instance++;
    snprintf(base_output_path, PATH_MAX, "%s%s.%s.%s.%d.%d", prefix, process_name, tool,
             host, (int)getpid(), instance);
  } while (access(base_output_path, F_OK)==0);

  LIBPAPIEX_DEBUG("Base output path is: %s\n", base_output_path);

  const char* path_container = ".papiex.output";
  unlink(path_container);
  /* for MPI programs, this function must only be called in the MASTER */
  if (is_mpied) {
    /* let's write the base output path to a file, so other processes can use that */
    /* share the root process's  base_output_path with the others */
    FILE* f = fopen(path_container, "w");
    if (f == NULL) {
      fprintf(stderr, "Could not create file (%s) that will contain the output name\n", path_container);
      return;
    }
    fprintf(f, "%s", base_output_path);
    fclose(f);
  }
}


static const char* get_base_output_path(char* prefix, char* user_spec_file) {
  // if we already have the path, return it:
  if (strlen(base_output_path) > 0) return base_output_path;
  if (is_mpied) {
    // let's see if path container exists
    if (!file_exists(path_container)) 
      LIBPAPIEX_DEBUG("Waiting for master to create path container file");
    while (!file_exists(path_container)) { sleep(1); }
    FILE *f = fopen(path_container, "r");
    if (fscanf(f,"%s", base_output_path)<1) {
      LIBPAPIEX_ERROR("Error reading base output path from path container\n");
    }
    LIBPAPIEX_DEBUG("Got base output path: %s\n", base_output_path);
  }
  else {
    set_base_output_path(prefix, user_spec_file);
  }
  return base_output_path;
}


// Only call this function after base_output_path has been set
// for MPI programs, this function can only be called after rank is set
static const char* get_proc_output_path(void) {
  if (strlen(proc_output_path) >0) return proc_output_path;
  if (is_mpied) {
    snprintf(proc_output_path,PATH_MAX, "%s/task_%d", base_output_path, myrank);
  }
  else { 
    strncpy(proc_output_path, base_output_path, PATH_MAX);
  }
  return proc_output_path;
}

const char* get_thread_output_path(int thread_id) {
  snprintf(thread_output_path, PATH_MAX, "%s/thread_%d%s", proc_output_path, thread_id, suffix);
  return thread_output_path;
}


/* This function can be called safely multiple times.
 * It creates the process path. 
 * For:
 *  unthreaded, non-mpi: No path created
 *    threaded, non-mpi: Process directory created (includes user-specified path as well)
 *  unthreaded,     mpi: Base directory created
 *    threaded,     mpi: Base->Process directory created
 */
void make_output_paths(int is_threaded) {
  if (is_mpied) {
    if (strlen(base_output_path)==0) {
      LIBPAPIEX_ERROR("Cannot make output path, when base output path is unset\n");
      monitor_real_exit(1);
    }
    LIBPAPIEX_DEBUG("This is an MPI program. Creating a base directory if it doesn't exist\n");
    if (mkdir(base_output_path, 0755)) {
      if (errno != EEXIST) {
        LIBPAPIEX_ERROR("Cannot created base directory: %s (errno=%d)\n", base_output_path, errno);
        exit(1);
      }
    }
  }
  if (is_threaded) {
    if (strlen(proc_output_path)==0) {
      get_proc_output_path();
    }
    LIBPAPIEX_DEBUG("This is a threaded program. Creating a process directory if it doesn't exist\n");
    if (mkdir(proc_output_path, 0755)) {
      if (errno != EEXIST) {
        LIBPAPIEX_ERROR("Cannot created process directory: %s (errno=%d)\n", proc_output_path, errno);
        exit(1);
      }
    }
  }
  return;
}


static inline const char* get_spec_file(void) {
  if (spec_file) return spec_file;
  if ((strcmp(hwinfo->vendor_string, "MIPS")==0) && (strncmp(hwinfo->model_string, "74K",3)==0))
    return MIPS74K_SPEC_FILE;
  if (strcmp(hwinfo->vendor_string, SC_VENDOR_STRING)==0) return ICE9_SPEC_FILE;
  if (is_intel_core_processor(hwinfo->vendor,hwinfo->cpuid_family,hwinfo->cpuid_model)) return NEHALEM_SPEC_FILE;
  return DEFAULT_SPEC_FILE;
}

static inline double sqdiff_ll(long long arg1, long long arg2)
{
  double ret, a, b;

  //printf("%d %lld %lld\n",get_rank(),arg1,arg2);
  a = (double)arg1;
  b = (double)arg2;
  
  ret = a - b;
  return (ret*ret);
}

/* This returns the unique index for the argument label in the process-specific hash.
 * If the lookup succeeds then the index is returned. If the hash lookup fails,
 * then the label is inserted into the hash, and a counter is incremented.
 */
/*
static int get_label_index(const char* label, struct hsearch_data *ht) {
  int retval = 0;
  static int next_idx = 1; // is reserved for the thread data
  ENTRY e, *ep = NULL;
  
  // first check if the label exists
  e.key = (char*)label;
  e.data = NULL;

  // never use non-reentrant versions of the below
  if (hsearch_r(e, FIND, &ep, ht) == 0) 
    {
      if (errno != ESRCH)
	LIBPAPIEX_ERROR("hsearch_t(FIND,%s) failed. %s\n",e.key,strerror(errno));
      e.key = strdup(label);
      e.data = (void*) next_idx;
      retval = next_idx;
      next_idx++;
      if (hsearch_r(e, ENTER, &ep, ht) == 0) 
	LIBPAPIEX_ERROR("Cannot insert %s in label->index mapping hash. It's full!.\n"
			"Create a larger hash with hcreate, and try again\n", label);
      if (ep == NULL)
	LIBPAPIEX_ERROR("hsearch_t(ENTER,%s) succeeded, but returned NULL.\n",e.key);
    }
  else 
    {
      if (ep == NULL)
	LIBPAPIEX_ERROR("hsearch_r(FIND,%s) succeeded, but returned NULL.\n",e.key);
      retval = (int)ep->data;
    }
  LIBPAPIEX_DEBUG("Returned idx=%d, for label(%s)\n", retval, label);
  return retval;
}
*/


/* This compares label and sub_label. If they are both non-empty, then they
 * must match or an error (1) is returned. If the first is a string and the 
 * second is null, then an error (1) is returned. If the first is an empty
 * string then the second is copied into the first
 */
static inline int match_caliper_label(char *label, const char* sub_label) {
  assert(label);
  assert(sub_label);

  // common case first
  if ((label[0] != '\0') && (sub_label[0] != '\0')) return (strcmp(label, sub_label)); 

  if ((sub_label[0]=='\0') && (label[0] =='\0')) return 0;
  if ((sub_label[0]=='\0') && (label[0] !='\0')) return 1;
  
  if (label[0] =='\0') strcpy(label, sub_label); return 0;

}

// This function checks the existence of path,
// if it doesn't exist, 0 is returned, else, the
// next available generation is returned
static int get_next_gen(const char *path) {
  struct stat buf;
  char tmp_path[PATH_MAX];
  strcpy(tmp_path, path);
  int rc = stat(tmp_path, &buf);
  if ((rc<0) && (errno==ENOENT)) return 0;
  int inst = 0;
  do {
    inst++;
    strcpy(tmp_path, path);
    sprintf(tmp_path, "%s.%d", path, inst);
    rc = stat(tmp_path, &buf);
  } while (!((rc<0) && (errno==ENOENT))); 
  return inst;
}

/* This is a function with overloaded arguments. Use with care! 
 * desc - String printed before the dots 
 * count - Number (can be float or long). type should be TYPE_FLOAT or TYPE_LONG, else this count is ignored (TYPE_STRING)
 * type - One of TYPE_{SRING|LONG_FLOAT}
 * make_indented - Leave blanks at the begining of the output line
 * no_newline - No newline is printed after the output line if this is set.
 * str - Printed when TYPE_STRING is used.
 */

void pretty_print(FILE* output, const char* desc, double count, int type, int make_indented, int no_newline, const char* str) {
  int pad_len = PAD_LENGTH;
  char ndesc[PATH_MAX];
  strcpy(ndesc, desc);
  ndesc[PAD_LENGTH-2] = '\0'; // truncate ndesc to the requisite length. strncpy behaves weird.

  if (make_indented) {
    pad_len-=4;
    fprintf(output, "    ");
  }
  //if (strchr(desc, '%'))
  //  pad_len++;
  if (quiet || papiex_nextgen) {
    if (type==TYPE_LONG)
      fprintf(output, "%-16lld %s", (long long) count, ndesc);
    else if (type==TYPE_FLOAT)
      fprintf(output, "%-16.2f %s", (float)count, ndesc);
  }
  else {
    /* not quiet */
#ifdef NO_PRETTY_PRINT
    if (type==TYPE_LONG) {
      if (no_scientific)
        fprintf(output, "%-30s: %16lld", ndesc, (long long) count);
      else
        fprintf(output, "%-30s: %16g", ndesc, (double) count);
    }
    else if (type==TYPE_FLOAT){
      fprintf(output, "%-30s: %16.2f", ndesc,  (float)count);
    }
#else
    /* The old dot dot pretty print */
    fprintf(output, "%s ", ndesc);
    int len;
    for (len=strlen(ndesc)+1; len<pad_len; len++)
      fprintf(output, ".");
    if (type==TYPE_LONG) {
      if (no_scientific)
        fprintf(output, " %16lld", (long long)count);
      else
        fprintf(output, " %16g", (double)count);
    }
    else if (type==TYPE_FLOAT){
      fprintf(output, " %16.2f", (float)count);
    }
    else if (type==TYPE_STRING){
      fprintf(output, " %16s", str);
    }
#endif 
  }
  if (!no_newline) fprintf(output, "\n");
  return;
}

static inline long long get_event_count(const char* event_name, const papiex_perthread_data_t *thread, unsigned int index) {
  long long retval = -1;
  int idx = get_event_index(event_name,(const char**) eventnames, eventcnt);
  if (idx >= 0) 
    retval = thread->data[index].counters[idx];
  return retval; 
}

// Failsafe function to get cycles. Will first try to return the hardware event count.
/// Failing that, it will give the next best value
static inline long long get_cpu_cycles(const papiex_perthread_data_t* thread, int index) {
  long long retval = get_event_count("PAPI_TOT_CYC", thread, index);
  if (retval < 0) {
#ifdef FULL_CALIPER_DATA
    retval = thread->data[index].virt_cyc;
#else
    /* For calipers we may not have virt_cyc available */
    if (index) 
      retval = thread->data[index].real_cyc;
    else
      retval = thread->data[index].virt_cyc;
#endif
  }
  
  return retval;
}

/*
static inline float get_virt_usec(papiex_perthread_data_t* thread, int index) {
  long long virt_cyc =  get_cpu_cycles(thread, index);
  if ((virt_cyc > 0) && hwinfo && (hwinfo->mhz > 0))
    return ((float)virt_cyc /(float) hwinfo->mhz);
  else 
    return ((float)thread->data[index].virt_usec);
}
*/

/* Returns either the stored value of real_usec, or it calculates
 * using real_cyc (for calipers when FULL_CALIPER_DATA is unset.
 */
static inline float get_real_usec(papiex_perthread_data_t* thread, int index) {
  float real_usec = 0;
#ifndef FULL_CALIPER_DATA
  if (index==0)
#endif
  {
    real_usec = (float)thread->data[index].real_usec;
    return real_usec;
  }
  /* caliper (index >0) && FULL_CALIPER_DATA unset */
  if (hwinfo->mhz > 0) {
    long long real_cyc = thread->data[index].real_cyc;
    real_usec = (float)real_cyc /(float) hwinfo->mhz;
  }
  return real_usec;
}

static int print_event_count(FILE *output, const char* desc, const char* event, 
                                     const papiex_perthread_data_t *thread, unsigned int index) {
  int  idx = get_event_index(event, (const char**) eventnames, eventcnt); 
  int pad_len = PAD_LENGTH;
  if (index > 0)    /*index calipers*/
    pad_len-=4;
  int len;
  if (idx >= 0) {
      if (quiet) {
        if (index > 0)                 /* indent the caliper */
          fprintf(output, "    ");
        fprintf(output, "%-16lld %s", thread->data[index].counters[idx], desc);
      }
      else {
        /* !quiet */
        pretty_print(output, desc, thread->data[index].counters[idx], 1, index, 1, NULL);
        // print % of thread for calipers
        if (index > 0)
          fprintf(output, "\t[%5.1f%%]", 100.0*(float)thread->data[index].counters[idx]/(float)thread->data[0].counters[idx]);
      }
      fprintf(output, "\n");
  }     
  else if (_papiex_debug) {
      if (index > 0)                 /* index calipers */
        fprintf(output, "    ");
      fprintf(output, "%s ", desc);
      for (len=strlen(desc)+1; len<pad_len; len++)
        fprintf(output, ".");
      fprintf(output, "       Not Available\n");
  }
  return idx;
}

static inline long long min(long long a, long long b) {
  if (a <= b) return a;
  else return b;
}

static inline const char* open_flags2str(int flags) {
  char ret_str[PATH_MAX];
  ret_str[0] = '\0';
  if (flags & O_RDONLY) strcat (ret_str, "O_RDONLY|");
  if (flags & O_WRONLY) strcat (ret_str, "O_WRONLY|");
  if (flags & O_RDWR) strcat (ret_str, "O_RDWR|");
  if (flags & O_CREAT) strcat (ret_str, "O_CREAT|");
  if (flags & O_EXCL) strcat (ret_str, "O_EXCL|");
  if (flags & O_NOCTTY) strcat (ret_str, "O_NOCTTY|");
  if (flags & O_TRUNC) strcat (ret_str, "O_TRUNC|");
  if (flags & O_APPEND) strcat (ret_str, "O_APPEND|");
  if (flags & O_NONBLOCK) strcat (ret_str, "O_NONBLOCK|");
  if (flags & O_NDELAY) strcat (ret_str, "O_NDELAY|");
  if (flags & O_SYNC) strcat (ret_str, "O_SYNC|");
  if (flags & O_FSYNC) strcat (ret_str, "O_FSYNC|");
  if (flags & O_ASYNC) strcat (ret_str, "O_ASYNC|");
  int len = strlen(ret_str);
  if (len >0) ret_str[len-1] = '\0';
  return (strdup(ret_str));
}

static inline const char* mode2string(int mode) {
  char ret_str[PATH_MAX];
  ret_str[0] = '\0';
  if (mode & S_IRWXU) strcat (ret_str, "S_IRWXU | ");
  if (mode & S_IRUSR) strcat (ret_str, "S_IRUSR | ");
  if (mode & S_IXUSR) strcat (ret_str, "S_IXUSR | ");
  if (mode & S_IRWXG) strcat (ret_str, "S_IRWXG | ");
  if (mode & S_IRGRP) strcat (ret_str, "S_IRGRP | ");
  if (mode & S_IWGRP) strcat (ret_str, "S_IWGRP | ");
  if (mode & S_IXGRP) strcat (ret_str, "S_IXGRP | ");
  if (mode & S_IRWXO) strcat (ret_str, "S_IRWXO | ");
  if (mode & S_IROTH) strcat (ret_str, "S_IROTH | ");
  if (mode & S_IWOTH) strcat (ret_str, "S_IWOTH | ");
  if (mode & S_IXOTH) strcat (ret_str, "S_IXOTH | ");
  return (strdup(ret_str));
}

static inline long long max(long long a, long long b) {
  if (a >= b) return a;
  else return b;
}

static void print_executable_info(FILE *output) {
  char tool_version[128];
  char tool_build[128];
  sprintf(tool_version, "%s version", tool);
  sprintf(tool_build, "%s build", tool);
  fprintf(output,"%-30s: %s\n", tool_version, PAPIEX_VERSION);
  fprintf(output,"%-30s: %s/%s\n", tool_build, __DATE__, __TIME__);
  fprintf(output,"%-30s: %s\n", "Executable", fullname);
  fprintf(output,"%-30s: %s\n", "Arguments", process_args);
  fprintf(output,"%-30s: %s\n", "Processor", processor);
  fprintf(output,"%-30s: %f\n", "Clockrate (MHz)", clockrate);

  fprintf(output,"%-30s: %s\n", "Hostname", hostname);
  fprintf(output,"%-30s: %s\n", "Options", getenv(PAPIEX_ENV));
#ifdef HAVE_PAPI
  fprintf(output,"%-30s: %s\n", "Domain", stringify_domain(domain));
#endif
  fprintf(output,"%-30s: %d\n", "Parent process id", (int)getppid());
  fprintf(output,"%-30s: %d\n", "Process id", (int)getpid());
  return;
}

static void print_rusage_stats(FILE *output) {
  struct rusage ru;
  getrusage(RUSAGE_SELF,&ru);
  pretty_printl(output, "Max resident", 0, ru.ru_maxrss);
  pretty_printl(output, "Shared", 0, ru.ru_ixrss);
  pretty_printl(output, "Data", 0, ru.ru_idrss);
  pretty_printl(output, "Stack", 0, ru.ru_isrss);
  pretty_printl(output, "Min faults", 0, ru.ru_minflt);
  pretty_printl(output, "Page faults", 0, ru.ru_majflt);
  pretty_printl(output, "Swaps", 0, ru.ru_nswap);
  pretty_printl(output, "Block dev inputs", 0, ru.ru_inblock);
  pretty_printl(output, "Block dev outputs", 0, ru.ru_oublock);
  pretty_printl(output, "Msgs sent", 0, ru.ru_msgsnd);
  pretty_printl(output, "Msgs received", 0, ru.ru_msgrcv);
  pretty_printl(output, "Signals", 0, ru.ru_nsignals);
  pretty_printl(output, "Voluntary yields", 0, ru.ru_nvcsw);
  pretty_printl(output, "Preemptions", 0, ru.ru_nivcsw);
  return;
}

static void print_derived_stats(FILE* output, papiex_perthread_data_t *thread, unsigned int index, 
                                const int scope, float process_walltime_usecs) {
    LIBPAPIEX_DEBUG("Printing derived statistics in %s\n", SCOPE_STRING[scope]);
    char desc[PATH_MAX] = "";
    static int not_first_pass = 0;
    int current_exceptions = 0;
    int current_rounding = 0;

    if (no_derived_stats)
      return;

    /* Disable floating point traps. Tushar's code can divide by zero. Ugh. */
    current_exceptions = fegetexcept();
    current_rounding = fegetround();
    fedisableexcept(FE_ALL_EXCEPT);
    fesetround(FE_TOWARDZERO);

    if (index && !quiet) fprintf(output, "    ");
    if (!quiet) fprintf(output, "Derived Metrics:\n");
    if (index && !quiet) fprintf(output, "    ");
    if (!quiet) fprintf(output, "---------------:\n");


    // Using PAPI presets
    { 
       if ((index==0) && ((scope == JOB_SCOPE) || (scope == PROC_SCOPE))) { 
         if (get_event_index("PAPI_FP_OPS",(const char**) eventnames, eventcnt) >= 0) {
           pretty_printf(output, "MFLOPS Aggregate (wallclock)", index,
           ((float)get_event_count("PAPI_FP_OPS", thread, index)
                /   ((float)((1000000 * proc_fini_time.tv_sec + proc_fini_time.tv_usec) - 
                            (1000000 * proc_init_time.tv_sec + proc_init_time.tv_usec)))));
          ADD_DERIVED_DESC("MFLOPS Aggregate (wallclock)", "Aggregate flops across tasks/threads per _wallclock_ second");
          ADD_DERIVED_DESC("", "= PAPI_FP_OPS/WALL_CLOCK_USEC");
        }
      }
      if (get_event_index("PAPI_FP_OPS",(const char**) eventnames, eventcnt) >= 0) {
        pretty_printf(output, "MFLOPS", index,
          ((float)get_event_count("PAPI_FP_OPS", thread, index)/(float)get_real_usec(thread,index)));
        ADD_DERIVED_DESC("MFLOPS", "PAPI_FP_OPS / Real usecs");
      }
      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "IPC", index,
          ((float)get_event_count("PAPI_TOT_INS", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index)));
	ADD_DERIVED_DESC("IPC", "PAPI_TOT_INS / PAPI_TOT_CYC");
      }

      /* Time spent */
      if (!quiet) fprintf(output, "\n");
      if (index && !quiet) fprintf(output, "    ");
      if (!quiet) fprintf(output, "Time:\n");
      if ((index == 0) && ((scope == PROC_SCOPE) || (scope== JOB_SCOPE))) {
          pretty_printf(output, "Wallclock (sec)", index, 
	       (float)((1000000 * proc_fini_time.tv_sec + proc_fini_time.tv_usec) - 
                              (1000000 * proc_init_time.tv_sec + proc_init_time.tv_usec))/ 1000000);
	  ADD_DERIVED_DESC("Wallclock (sec)", "Wallclock time in seconds");
      }
/*
      if (! printed_utilization) {
        float virt_usec = thread->data[index].virt_usec;
        if (virt_usec <= 0) virt_usec = get_virt_usec(thread, index);
        pretty_printf(output, "Running Time %", index,
	  (100.0 *(float)virt_usec/(float)thread->data[index].real_usec));
	ADD_DERIVED_DESC("Running Time %", "100 * Virtual usecs / Real usecs");
        printed_utilization = 1;
      }
*/
      if (get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) {
         pretty_printf(output, "Running Time in Domain %", index,
          (100.0 * (float)get_event_count("PAPI_TOT_CYC", thread, index)/(float)thread->data[index].real_cyc));
         ADD_DERIVED_DESC("Running Time in Domain %", "100 * PAPI_TOT_CYC / Real cycles");
      }

      /* Instruction mix */
      if (!quiet) fprintf(output, "\n");
      if (index && !quiet) fprintf(output, "    ");
      if (!quiet) fprintf(output, "Instructions:\n");
      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_LST_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Memory Instructions %", index,
	  ((float)100*(get_event_count("PAPI_LST_INS", thread, index))
	           /(float)get_event_count("PAPI_TOT_INS", thread, index)));
      }
      else {
        if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_LD_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_SR_INS",(const char**) eventnames, eventcnt) >= 0)) {
          pretty_printf(output, "Memory Instructions %", index,
	  ((float)100*(get_event_count("PAPI_LD_INS", thread, index)+
                       get_event_count("PAPI_SR_INS", thread, index))
	           /(float)get_event_count("PAPI_TOT_INS", thread, index)));
        }
      }
      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_FP_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "FP Instructions %", index,
	  ((float)100*get_event_count("PAPI_FP_INS", thread, index) /(float)get_event_count("PAPI_TOT_INS", thread, index)));
      }
      else if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_FP_OPS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "FP Instructions (approx) %", index,
	  ((float)100*get_event_count("PAPI_FP_OPS", thread, index) /(float)get_event_count("PAPI_TOT_INS", thread, index)));
         ADD_DERIVED_DESC("FP Instructions (approx) %", "100 * PAPI_FP_OPS / PAPI_TOT_CYC");
      }

      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_BR_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Branch Instructions %", index,
	  ((float)100*get_event_count("PAPI_BR_INS", thread, index) /(float)get_event_count("PAPI_TOT_INS", thread, index)));
      }

      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_INT_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Integer Instructions %", index,
	  ((float)100*get_event_count("PAPI_INT_INS", thread, index) /(float)get_event_count("PAPI_TOT_INS", thread, index)));
      }

      /* Memory */
      if (!quiet) fprintf(output, "\n");
      if (index && !quiet) fprintf(output, "    ");
      if (!quiet) fprintf(output, "Memory:\n");
      if ((get_event_index("PAPI_LD_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_SR_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Loads/Stores ratio", index,
	  ((float)get_event_count("PAPI_LD_INS", thread, index) /(float)get_event_count("PAPI_SR_INS", thread, index)));
      }

      if ((get_event_index("PAPI_L1_DCM",(const char**) eventnames, eventcnt) >= 0) && 
	   (get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "L1 D-cache Misses Per Thousand Ins.", index,
	   ((float)1000*((float)get_event_count("PAPI_L1_DCM", thread, index)  
	   /(float)get_event_count("PAPI_TOT_INS", thread, index))));
      }

      if ((get_event_index("PAPI_LST_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_L1_DCM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "L1 D-cache hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_L1_DCM", thread, index) 
	               /(float)(get_event_count("PAPI_LST_INS", thread, index))))));
      }
      else if ((get_event_index("PAPI_L1_DCA",(const char**) eventnames, eventcnt) >= 0)
        && (get_event_index("PAPI_L1_DCM",(const char**) eventnames, eventcnt) >= 0)){
        pretty_printf(output, "L1 D-cache hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_L1_DCM", thread, index) 
	               /(float)(get_event_count("PAPI_L1_DCA", thread, index))))));
      }

      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_L1_ICM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "L1 I-cache hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_L1_ICM", thread, index) 
	               /(float)(get_event_count("PAPI_TOT_INS", thread, index))))));
      }

      if ((get_event_index("PAPI_L2_DCA",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_L2_DCM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "L2 D-cache hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_L2_DCM", thread, index) 
	               /(float)(get_event_count("PAPI_L2_DCA", thread, index))))));
      }

      if ((get_event_index("PAPI_L2_ICA",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_L2_ICM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "L2 I-cache hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_L2_ICM", thread, index) 
	               /(float)(get_event_count("PAPI_L2_ICA", thread, index))))));
      }

      if ((get_event_index("PAPI_LST_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_TLB_DM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "D-TLB hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_TLB_DM", thread, index) 
	               /(float)(get_event_count("PAPI_LST_INS", thread, index))))));
      }

      if ((get_event_index("PAPI_TOT_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_TLB_IM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "I-TLB hit %", index,
          (100.00- 
	  ((float)100*((float)get_event_count("PAPI_TLB_IM", thread, index) 
	               /(float)(get_event_count("PAPI_TOT_INS", thread, index))))));
      }

      /* Stalls */
      if (!quiet) fprintf(output, "\n");
      if (index && !quiet) fprintf(output, "    ");
      if (!quiet) fprintf(output, "Stalls:\n");
      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_RES_STL",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Stall Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_RES_STL", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }

      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_MEM_SCY",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Memory Stall Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_MEM_SCY", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }

      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_FP_STAL",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "FP Stall Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_FP_STAL", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }

      if (!quiet) fprintf(output, "\n");
      if ((get_event_index("PAPI_FP_OPS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_L1_DCM",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "FLOPS per D-cache miss", index,
	  ((float)((float)get_event_count("PAPI_FP_OPS", thread, index)/(float)get_event_count("PAPI_L1_DCM", thread, index))));
      }

      if ((get_event_index("PAPI_FP_OPS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_LST_INS",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Computational Intensity (FLOP/load-store)", index,
	  ((float)(get_event_count("PAPI_FP_OPS", thread, index) /(float)get_event_count("PAPI_LST_INS", thread, index))));
      }

      if ((get_event_index("PAPI_BR_INS",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_BR_MSP",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Branch Misprediction %", index,
	  ((float)100*((float)get_event_count("PAPI_BR_MSP", thread, index)/(float)get_event_count("PAPI_BR_INS", thread, index))));
      }

      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_FUL_ICY",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "Full Issue Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_FUL_ICY", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }

      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_STL_ICY",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "No Issue Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_STL_ICY", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }

      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_FPU_IDL",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "FPU Idle Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_FPU_IDL", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }

      if ((get_event_index("PAPI_TOT_CYC",(const char**) eventnames, eventcnt) >= 0) 
        && (get_event_index("PAPI_LSU_IDL",(const char**) eventnames, eventcnt) >= 0)) {
        pretty_printf(output, "LSU Idle Cycles %", index,
	  ((float)100*((float)get_event_count("PAPI_LSU_IDL", thread, index)/(float)get_event_count("PAPI_TOT_CYC", thread, index))));
      }
    }

  // common to all architectures
  if (index == 0) {
    if (!no_mpi_prof && is_mpied) {
      pretty_printf(output, "MPI cycles %", index,
        (thread->data[0].real_cyc ? (float)((float)100*thread->papiex_mpiprof/(float)thread->data[0].real_cyc) : 0));
      ADD_DERIVED_DESC("MPI cycles %", "100 * Real cycles in MPI / Real cycles");
      pretty_printf(output, "MPI Sync cycles %", index,
        (thread->data[0].real_cyc ? (float)((float)100*thread->papiex_mpisyncprof/(float)thread->data[0].real_cyc) : 0));
      ADD_DERIVED_DESC("MPI Sync cycles %", "100 * Real cycles in MPI / Real cycles");
    }
    if (!no_io_prof) {
      pretty_printf(output, "IO cycles %", index,
        (thread->data[0].real_cyc ? (float)((float)100*thread->papiex_ioprof/(float)thread->data[0].real_cyc) : 0));
      ADD_DERIVED_DESC("IO cycles %", "100 * Real cycles in I/O / Real cycles");
    }
    if (!no_threadsync_prof) {
      pretty_printf(output, "Thr Sync cycles %", index,
        (thread->data[0].real_cyc ? (float)((float)100*thread->papiex_threadsyncprof/(float)thread->data[0].real_cyc) : 0));
      ADD_DERIVED_DESC("Thr Sync cycles %", "100 * Real cycles in I/O / Real cycles");
    }
    ADD_DERIVED_DESC("Virtual","Counted only when process (or kernel on behalf of procss) executing on CPU")
    ADD_DERIVED_DESC("Real","Always counted, unhalted time");
  } // all architectures
  fprintf(output, "\n");

  not_first_pass++; // hack needed to make sure we don't append the same derived description twice
  feenableexcept(current_exceptions);
  fesetround(current_rounding);
}

static void print_pretty_stats(FILE* output, papiex_perthread_data_t *thread, unsigned int index, 
                               const int scope, float process_walltime_usecs) {
    LIBPAPIEX_DEBUG("Printing pretty stats in %s\n", SCOPE_STRING[scope]);
   
    /* index 0 is special -- it has counts for the thread */
    LIBPAPIEX_DEBUG("Printing pretty stats for caliper %d of thread\n", index);
    if (index == 0) {
      if (multiplex && (thread->data[0].virt_usec <= ((1000000 * PAPIEX_MPX_MIN_SAMPLES)/mpx_interval))) {            //  so we get at least MIN_SAMPLES
        fprintf(output, 
               "*********************** WARNING *****************************\n"
               "This thread ran for %4.2f secs (CPU time) which is less than\n"
               "the time (%4.2f secs) needed to get the minimum %d samples.\n"
               "This makes the numbers suspect. You can try to either disable\n"
               "multiplexing (no -m or -a), or increase the multiplexing frequency\n"
               "from the current value of %d by using the -m<frequency> flag.\n"
               "*************************************************************\n\n",
                ((float)thread->data[0].virt_usec/1000000), ((float)PAPIEX_MPX_MIN_SAMPLES/mpx_interval), PAPIEX_MPX_MIN_SAMPLES, mpx_interval);
      }
    }
    else {
        if (thread->data[index].label && (strlen(thread->data[index].label) != 0))
	  fprintf(output,"    %s\n",thread->data[index].label);
	else
	  fprintf(output,"    Caliper %d\n",index);
    }

    print_derived_stats(output, thread, index, scope, process_walltime_usecs);
    if (multiplex) {
      if (index && !quiet) fprintf(output, "    ");
      if (!quiet) fprintf(output, "Multiplex Counts:\n");
    }

   /* This is using PAPI presets */
   print_event_count(output, "Cycles", "PAPI_TOT_CYC", thread, index);
   print_event_count(output, "Total Resource Stalls","PAPI_RES_STL",thread, index);
   print_event_count(output, "Memory Stall Cycles","PAPI_MEM_SCY",thread, index);
   print_event_count(output, "FP Stall Cycles","PAPI_FP_STAL",thread, index);
   print_event_count(output, "FP Operations","PAPI_FP_OPS",thread, index);
   print_event_count(output,"Instructions Completed", "PAPI_TOT_INS", thread, index);
   print_event_count(output, "FP Instructions","PAPI_FP_INS", thread, index);
   print_event_count(output, "Integer Instructions","PAPI_INT_INS",thread, index);
   print_event_count(output, "FMA Instructions","PAPI_FMA_INS",thread, index);
   print_event_count(output, "Vector Instructions","PAPI_VEC_INS",thread, index);
   print_event_count(output, "Load-Store Instructions","PAPI_LST_INS",thread, index);
   print_event_count(output, "Load Instructions","PAPI_LD_INS",thread, index);
   print_event_count(output, "Store Instructions","PAPI_SR_INS",thread, index);
   print_event_count(output, "Store Conditionals Issued","PAPI_CSR_TOT",thread, index);
   print_event_count(output, "Store Conditionals Failed","PAPI_CSR_FAL",thread, index);
   print_event_count(output, "Branch Instructions","PAPI_BR_INS",thread, index);
   print_event_count(output, "Conditional Branch Instructions","PAPI_BR_CN",thread, index);
   print_event_count(output, "Mispredicted Branches","PAPI_BR_MSP",thread, index);
   print_event_count(output, "L1 Data Cache Accesses","PAPI_L1_DCA",thread, index);
   print_event_count(output, "L1 Data Cache Misses","PAPI_L1_DCM",thread, index);
   print_event_count(output, "L1 Instruction Cache Misses","PAPI_L1_ICM",thread, index);
   print_event_count(output, "L2 Data Cache Accesses","PAPI_L2_DCA",thread, index);
   print_event_count(output, "L2 Data Cache Misses","PAPI_L2_DCM",thread, index);
   print_event_count(output, "L2 Instruction Cache Accesses","PAPI_L2_ICA",thread, index);
   print_event_count(output, "L2 Instruction Cache Misses","PAPI_L2_ICM",thread, index);
   print_event_count(output, "Cache Line Invalidation Requests","PAPI_CA_INV",thread, index);
   print_event_count(output, "Data TLB Misses","PAPI_TLB_DM",thread, index);
   print_event_count(output, "Instruction TLB Misses","PAPI_TLB_IM",thread, index);
   print_event_count(output,"FPU Idle Cycles","PAPI_FPU_IDL",thread, index);
   print_event_count(output,"LSU Idle Cycles","PAPI_LSU_IDL",thread, index);
   print_event_count(output,"Cycles with No Issue","PAPI_STL_ICY",thread, index);
   print_event_count(output,"Cycles with Full Issue","PAPI_FUL_ICY",thread, index);

   fprintf(output, "\n");
   return;
}

/* Print pretty stats for the thread and all the caliper points */
static inline void print_all_pretty_stats(FILE* output, papiex_perthread_data_t *thread, 
                                          const int scope, float process_walltime_usecs) {
  int j;
  print_pretty_stats(output, thread, 0, scope, process_walltime_usecs);
  for (j=1;j<thread->max_caliper_entries;j++)
    if (thread->data[j].used >0)
      print_pretty_stats(output, thread, j, scope, process_walltime_usecs);
}


static void print_counters(FILE *output, papiex_perthread_data_t *thread) {
#ifdef HAVE_PAPI
  int i,j;
  if (quiet || papiex_nextgen) {
    if (multiplex && (thread->data[0].virt_usec <= ((1000000 * PAPIEX_MPX_MIN_SAMPLES)/mpx_interval))) {
      // make sure you have an empty line after the warning 
      // so the report parser can pick the start and finish of
      // the warning.
      fprintf(output, "\n--- WARNING ---\n"
               "This thread ran for %4.2f secs (CPU time), which is less than the time\n"
               "(%4.2f secs) needed to get the minimum %d samples. The numbers are suspect.\n"
               "Try the run without multiplex (without -a and -m) or increase the multiplex\n"
	       "frequency. You can increase the frequency from the current %d by specifying\n"
               "-m<interval> to get more samples.\n\n",
                ((float)thread->data[0].virt_usec/1000000), ((float)PAPIEX_MPX_MIN_SAMPLES/mpx_interval), PAPIEX_MPX_MIN_SAMPLES, mpx_interval);
      }
    fprintf(output,"%-16lld [PROCESS] Wallclock usecs\n", (long long int)((1000000 * proc_fini_time.tv_sec + proc_fini_time.tv_usec) -
                            (1000000 * proc_init_time.tv_sec + proc_init_time.tv_usec)));

    fprintf(output,"%-16lld Real usecs\n",thread->data[0].real_usec);
    fprintf(output,"%-16lld Real cycles\n",thread->data[0].real_cyc);
    fprintf(output,"%-16lld Virtual usecs\n",thread->data[0].virt_usec);
    fprintf(output,"%-16lld Virtual cycles\n",thread->data[0].virt_cyc);
    if (!no_mpi_prof && is_mpied) {
      fprintf(output,"%-16lld MPI cycles\n",thread->papiex_mpiprof);
      fprintf(output,"%-16lld MPI Sync cycles\n",thread->papiex_mpisyncprof);
    }
    if (!no_io_prof)
      fprintf(output,"%-16lld IO cycles\n",thread->papiex_ioprof);
    if (!no_threadsync_prof)
      fprintf(output,"%-16lld Thr Sync cycles\n",thread->papiex_threadsyncprof);
    for (i=0;i<eventcnt;i++)
      fprintf(output,"%-16lld %s\n",thread->data[0].counters[i],eventnames[i]);
      for (j=1;j<thread->max_caliper_entries;j++) {
        if (thread->data[j].used >0) {
          const char* label = thread->data[j].label ;
          fprintf(output,"\n%-16lld [LABEL] %s\n",(long long)j, label);

          /* if the user didn't set a label string use the calliper index */
          char index_s[32];
          if (strlen(thread->data[j].label) == 0) {
            snprintf(index_s, sizeof(index_s), "%d", j);
            label = index_s;
          }

          fprintf(output,"%-16lld [%s] Measurements\n",(long long)thread->data[j].used, label);
          fprintf(output,"%-16lld [%s] Real cycles\n",thread->data[j].real_cyc, label);
#ifdef FULL_CALIPER_DATA
          fprintf(output,"%-16lld [%s] Real usecs\n",thread->data[j].real_usec, label);
          fprintf(output,"%-16lld [%s] Virtual usecs\n",thread->data[j].virt_usec, label);
          fprintf(output,"%-16lld [%s] Virtual cycles\n",thread->data[j].virt_cyc, label);
          if (!no_mpi_prof && is_mpied) {
            fprintf(output,"%-16lld [%s] MPI cycles\n",thread->data[j].mpiprof, label);
            fprintf(output,"%-16lld [%s] MPI Sync cycles\n",thread->data[j].mpisyncprof, label);
          }
          if (!no_io_prof)
            fprintf(output,"%-16lld [%s] IO cycles\n",thread->data[j].ioprof, label);
          if (!no_threadsync_prof)
            fprintf(output,"%-16lld [%s] Thr Sync cycles\n",thread->data[j].threadsyncprof, label);
#endif
          for (i=0;i<eventcnt;i++)
            fprintf(output,"%-16lld [%s] %s\n",thread->data[j].counters[i], label, eventnames[i]);
	}
      }
  } /* quiet */

  else {
    /* not quiet */
    pretty_printl(output,"Real usecs",0,thread->data[0].real_usec);
    pretty_printl(output,"Real cycles",0,thread->data[0].real_cyc);
    pretty_printl(output,"Virtual usecs",0,thread->data[0].virt_usec);
    pretty_printl(output,"Virtual cycles",0,thread->data[0].virt_cyc);
    if (!no_mpi_prof && is_mpied) {
      pretty_printl(output,"MPI cycles",0,thread->papiex_mpiprof);
      pretty_printl(output,"MPI Sync cycles",0,thread->papiex_mpisyncprof);
      //if (!no_derived_stats) {
      //  fprintf(output,"%% MPI cycles:\t%16.2f\n",(thread->data[0].real_cyc ? (float)((float)100*thread->papiex_mpiprof/(float)thread->data[0].real_cyc) : 0));
      //}
    }
    if (!no_io_prof) {
      pretty_printl(output,"IO cycles",0,thread->papiex_ioprof);
      //if (!no_derived_stats) {
      //  fprintf(output,"%% IO cycles:\t\t%16.2f\n",(thread->data[0].real_cyc ? (float)((float)100*thread->papiex_ioprof/(float)thread->data[0].real_cyc) : 0));
      //}
    }
    if (!no_threadsync_prof) {
      pretty_printl(output,"Thr Sync cycles",0,thread->papiex_threadsyncprof);
    }
    for (i=0;i<eventcnt;i++) {
      pretty_printl(output, eventnames[i], 0, thread->data[0].counters[i]);
    }

    //for (j=1;j<thread->max_caliper_entries;j++)
    //  if (thread->data[j].used) {
    //    fprintf(output,"\n");
    //    break;
    //  }

    for (j=1;j<thread->max_caliper_entries;j++) {
      if (thread->data[j].used >0) {
        fprintf(output, "\n");
        if (strlen(thread->data[j].label) == 0)
	  fprintf(output,"    Caliper %d\n",j);
	else
	  fprintf(output,"    %s\n",thread->data[j].label);
        pretty_printl(output,"Executions",1, (long long)thread->data[j].used);
        pretty_printl(output,"Real cycles",1, thread->data[j].real_cyc);
#ifdef FULL_CALIPER_DATA
        pretty_printl(output,"Real usecs",1, thread->data[j].real_usec);
        pretty_printl(output,"Virtual usecs",1, thread->data[j].virt_usec);
        pretty_printl(output,"Virtual cycles",1, thread->data[j].virt_cyc);
        if (!no_mpi_prof && is_mpied) {
          pretty_printl(output,"MPI cycles",0,thread->data[j].mpiprof);
          pretty_printl(output,"MPI Sync cycles",0,thread->data[j].mpisyncprof);
        }
        if (!no_io_prof)
          pretty_printl(output,"IO cycles",0,thread->data[j].ioprof);
        if (!no_threadsync_prof)
          pretty_printl(output,"Thr Sync cycles",0,thread->data[j].threadsyncprof);
#endif
        for (i=0;i<eventcnt;i++) {
          pretty_print(output, eventnames[i],thread->data[j].counters[i], 1, 1, 1, NULL);
	  /* Derived check here */
          fprintf(output, " [%5.1f%%]\n", 100*(float)thread->data[j].counters[i]/(float)thread->data[0].counters[i]);
        }
      }
    }
  } /* not quiet end */

  /* dump event descriptions */
  if (!quiet) {
    fprintf(output,"\n");
    fprintf(output,"Event descriptions:\n");
    for (i=0;i<eventcnt;i++)
      _papiex_dump_event_info(output,eventcodes[i],0);

    fprintf(output, "%s", derived_events_desc);
  }
#endif
  return;
}


static void print_min_max_mean_cv(FILE *output, 
                                         papiex_perthread_data_t *min_data,
                                         papiex_perthread_data_t *max_data,
                                         papiex_perthread_data_t *mean_data,
                                         papiex_perthread_data_t *stddev_data) {
  int j,k;
  fprintf(output, "\n");
  if (quiet) {
    fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tReal cycles\n",
         min_data->data[0].real_cyc, max_data->data[0].real_cyc, 
         mean_data->data[0].real_cyc, (float)stddev_data->data[0].real_cyc/mean_data->data[0].real_cyc);
    fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tReal usecs\n",
       min_data->data[0].real_usec, max_data->data[0].real_usec, 
       mean_data->data[0].real_usec, (float)stddev_data->data[0].real_usec/mean_data->data[0].real_usec);
    fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tVirtual cycles\n",
       min_data->data[0].virt_cyc, max_data->data[0].virt_cyc, 
       mean_data->data[0].virt_cyc, (float)stddev_data->data[0].virt_cyc/mean_data->data[0].virt_cyc);
    fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tVirtual usecs\n",
       min_data->data[0].virt_usec, max_data->data[0].virt_usec, 
       mean_data->data[0].virt_usec, (float)stddev_data->data[0].virt_usec/mean_data->data[0].virt_usec);
    if (!no_mpi_prof && is_mpied) {
      fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tMPI cycles\n",
         min_data->papiex_mpiprof, max_data->papiex_mpiprof, 
         mean_data->papiex_mpiprof, (float)stddev_data->papiex_mpiprof/mean_data->papiex_mpiprof);
      fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tMPI Sync cycles\n",
         min_data->papiex_mpisyncprof, max_data->papiex_mpisyncprof, 
         mean_data->papiex_mpisyncprof, (float)stddev_data->papiex_mpisyncprof/mean_data->papiex_mpisyncprof);
    }
    if (!no_io_prof)
      fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tIO cycles\n",
         min_data->papiex_ioprof, max_data->papiex_ioprof, 
         mean_data->papiex_ioprof, (float)stddev_data->papiex_ioprof/mean_data->papiex_ioprof);
    if (!no_threadsync_prof)
      fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tThr Sync cycles\n",
         min_data->papiex_threadsyncprof, max_data->papiex_threadsyncprof, 
         mean_data->papiex_threadsyncprof, (float)stddev_data->papiex_threadsyncprof/mean_data->papiex_threadsyncprof);
    for (k=0;k<eventcnt;k++)
      fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\t%s\n", 
         min_data->data[0].counters[k], max_data->data[0].counters[k],
         mean_data->data[0].counters[k], (float)stddev_data->data[0].counters[k]/mean_data->data[0].counters[k],
         eventnames[k]);
    for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
      if (mean_data->data[j].used >0) {
        fprintf(output, "\nCaliper %d, %s:\n", j, (mean_data->data[j].label == NULL ? "" : mean_data->data[j].label));
        //fprintf(output,"%16lld\t%16lld\t%16lld\t%16lld\tMeasurements\n",
        //   (long long) min_data->data[j].used, (long long) max_data->data[j].used, 
        //   (long long)mean_data->data[j].used, (long long) stddev_data->data[j].used);
#ifdef FULL_CALIPER_DATA
        fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tReal usecs\n",
           (long long) min_data->data[j].real_usec, (long long) max_data->data[j].real_usec, 
           (long long)mean_data->data[j].real_usec, (float) stddev_data->data[j].real_usec/mean_data->data[j].real_usec);
        fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tVirtual usecs\n",
           (long long) min_data->data[j].virt_usec, (long long) max_data->data[j].virt_usec, 
           (long long)mean_data->data[j].virt_usec, (float) stddev_data->data[j].virt_usec/mean_data->data[j].virt_usec);
        fprintf(output,"%16lld\t%16lld\t%16lld\t%16.2f\tVirtual cycles\n",
           (long long) min_data->data[j].virt_cyc, (long long) max_data->data[j].virt_cyc, 
           (long long)mean_data->data[j].virt_cyc, (float) stddev_data->data[j].virt_cyc/mean_data->data[j].virt_cyc);
#endif
        for (k=0;k<eventcnt;k++)
          fprintf(output,"%s:\t%16lld\t%16lld\t%16lld\t%16.2f\n", eventnames[k], 
             min_data->data[0].counters[k], max_data->data[0].counters[k],
             mean_data->data[0].counters[k], (float)stddev_data->data[0].counters[k]/mean_data->data[0].counters[k]);
      }
    } /* calipers */
  }
  else {
      fprintf(output,"\nProgram Statistics:\n");
      fprintf(output,"%-30s: %12s\t%12s\t%12s\t%8s\n","Event","Min","Max","Mean","COV");
    /* print the global stats */
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Real usecs",
       (double)min_data->data[0].real_usec, (double)max_data->data[0].real_usec, 
       (double)mean_data->data[0].real_usec, (float)stddev_data->data[0].real_usec/mean_data->data[0].real_usec);

      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Real cycles",
         (double)min_data->data[0].real_cyc, (double)max_data->data[0].real_cyc, 
         (double)mean_data->data[0].real_cyc, (float)stddev_data->data[0].real_cyc/mean_data->data[0].real_cyc);
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Virtual usecs",
       (double)min_data->data[0].virt_usec, (double)max_data->data[0].virt_usec, 
       (double)mean_data->data[0].virt_usec, (float)stddev_data->data[0].virt_usec/mean_data->data[0].virt_usec);
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Virtual cycles",
       (double)min_data->data[0].virt_cyc, (double)max_data->data[0].virt_cyc, 
       (double)mean_data->data[0].virt_cyc, (float)stddev_data->data[0].virt_cyc/mean_data->data[0].virt_cyc);
    if (!no_mpi_prof && is_mpied) {
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","MPI cycles",
         (double)min_data->papiex_mpiprof, (double)max_data->papiex_mpiprof, 
         (double)mean_data->papiex_mpiprof, (mean_data->papiex_mpiprof > 0 ? (float)stddev_data->papiex_mpiprof/mean_data->papiex_mpiprof : 0));
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","MPI Sync cycles",
         (double)min_data->papiex_mpisyncprof, (double)max_data->papiex_mpisyncprof, 
         (double)mean_data->papiex_mpisyncprof, (mean_data->papiex_mpisyncprof > 0 ? (float)stddev_data->papiex_mpisyncprof/mean_data->papiex_mpisyncprof : 0));
    }
    if (!no_io_prof) {
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f \n","IO cycles",
         (double)min_data->papiex_ioprof, (double)max_data->papiex_ioprof, 
         (double)mean_data->papiex_ioprof, (mean_data->papiex_ioprof > 0 ? (float)stddev_data->papiex_ioprof/mean_data->papiex_ioprof : 0));
    }
    if (!no_threadsync_prof) {
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f \n","Thr Sync cycles",
         (double)min_data->papiex_threadsyncprof, (double)max_data->papiex_threadsyncprof, 
         (double)mean_data->papiex_threadsyncprof, (mean_data->papiex_threadsyncprof > 0 ? (float)stddev_data->papiex_threadsyncprof/mean_data->papiex_threadsyncprof : 0));
    }
    for (k=0;k<eventcnt;k++)
      fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n", eventnames[k], 
         (double)min_data->data[0].counters[k], (double)max_data->data[0].counters[k],
         (double)mean_data->data[0].counters[k], (mean_data->data[0].counters[k] > 0 ? (float)stddev_data->data[0].counters[k]/mean_data->data[0].counters[k] :0));
    for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
      if (mean_data->data[j].used >0) {
        fprintf(output, "\nCaliper %d: %s\n", j, (mean_data->data[j].label == NULL ? "" : mean_data->data[j].label));
        //fprintf(output,"%16lld\t%16lld\t%16lld\t%16lld\tMeasurements\n",
        //   (long long) min_data->data[j].used, (long long) max_data->data[j].used, 
        //   (long long)mean_data->data[j].used, (long long) stddev_data->data[j].used);
#ifdef FULL_CALIPER_DATA
        fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Real usecs",
           (double)min_data->data[j].real_usec, (double)max_data->data[j].real_usec, 
           (double)mean_data->data[j].real_usec, (mean_data->data[j].real_usec > 0 ? (float)stddev_data->data[j].real_usec/mean_data->data[j].real_usec :0));
        fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Virtual usec:",
           (double)min_data->data[j].virt_usec, (double)max_data->data[j].virt_usec, 
           (double)mean_data->data[j].virt_usec, (mean_data->data[j].virt_usec > 0 ? (float)stddev_data->data[j].virt_usec/mean_data->data[j].virt_usec : 0));
        fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n","Virtual cycles",
           (double)min_data->data[j].virt_cyc, (double)max_data->data[j].virt_cyc, 
           (double)mean_data->data[j].virt_cyc, (mean_data->data[j].virt_cyc > 0 ? (float)stddev_data->data[j].virt_cyc/mean_data->data[j].virt_cyc : 0));
#endif
        for (k=0;k<eventcnt;k++)
          fprintf(output,"%-30s: %12g\t%12g\t%12g\t%8.2f\n", eventnames[k], 
             (double)min_data->data[0].counters[k], (double)max_data->data[0].counters[k],
             (double)mean_data->data[0].counters[k], (mean_data->data[0].counters[k] > 0 ? (float)stddev_data->data[0].counters[k]/mean_data->data[0].counters[k] : 0));
      }
    } /* calipers */

    fprintf(output,"\n");
    //fprintf(output,"\nEvent descriptions:\n");
    //for (k=0;k<eventcnt;k++)
    //  _papiex_dump_event_info(output,eventcodes[k],0);

  } /* not quiet */

  return;
}

/* Returns the index of 'name' in eventnames array.
 * Returns -1 if the name is not found
 */
static inline int get_event_index(const char * name, const char *eventnames[], int count) {
  int i;
  assert(name);
  for (i=0; i<count; i++) {
    if (strcmp(eventnames[i], name)==0)
      return i;
  }

  // if we reach here, the event doesn't exist in the array
  return -1;
}


#ifdef HAVE_PAPI
static char *stringify_domain(int domain)
{
  static char output[256];
  output[0] = '\0';
  if (domain & PAPI_DOM_USER)
    strcat(output,"User");
  if (domain & PAPI_DOM_KERNEL)
    {
      if (strlen(output))
	strcat(output,",");
      strcat(output,"Kernel");
    }
  if (domain & PAPI_DOM_OTHER)
    {
      if (strlen(output))
	strcat(output,",");
      strcat(output,"Other");
    }
  if (domain & PAPI_DOM_SUPERVISOR)
    {
      if (strlen(output))
	strcat(output,",");
      strcat(output,"Supervisor");
    }
  return(output);
}

char *stringify_all_domains(int domains)
{
  static char buf[PAPI_HUGE_STR_LEN];
  int i, did = 0;
  buf[0] = '\0';

  for (i=PAPI_DOM_MIN;i<=PAPI_DOM_MAX;i=i<<1)
    if (domains&i)
      {
        if (did)
          strcpy(buf+strlen(buf),"|");
        strcpy(buf+strlen(buf),stringify_domain(domains&i));
        did++;
      }
  return(buf);
}

char *stringify_granularity(int granularity)
{
   switch (granularity) {
   case PAPI_GRN_THR:
      return ("PAPI_GRN_THR");
   case PAPI_GRN_PROC:
      return ("PAPI_GRN_PROC");
   case PAPI_GRN_PROCG:
      return ("PAPI_GRN_PROCG");
   case PAPI_GRN_SYS_CPU:
      return ("PAPI_GRN_SYS_CPU");
   case PAPI_GRN_SYS:
      return ("PAPI_GRN_SYS");
   default:
     return("Unrecognized granularity!");
   }
   return (NULL);
}

char *stringify_all_granularities(int granularities)
{
  static char buf[PAPI_HUGE_STR_LEN];
  int i, did = 0;

  buf[0] = '\0';
  for (i=PAPI_GRN_MIN;i<=PAPI_GRN_MAX;i=i<<1)
    if (granularities&i)
      {
        if (did)
          strcpy(buf+strlen(buf),"|");
        strcpy(buf+strlen(buf),stringify_granularity(granularities&i));
        did++;
      }

  return(buf);
}

#endif

#if 0
static int granularity = 0;

static char *stringify_granularity(int granularity)
{
  switch(granularity)
    {
    case PAPI_GRN_THR:
      return("PAPI_GRN_THR");
    case PAPI_GRN_PROC:
      return("PAPI_GRN_PROC");
    case PAPI_GRN_PROCG:
      return("PAPI_GRN_PROCG");
    case PAPI_GRN_SYS_CPU:
      return("PAPI_GRN_SYS_CPU");
    case PAPI_GRN_SYS:
      return("PAPI_GRN_SYS");
    default:
      abort();
    }
  return(NULL);
}
#endif

void papiex_stop(int point)
{
  papiex_perthread_data_t *thread = NULL;
  void *tmp = NULL;
  int i, retval;
  long long counters[PAPIEX_MAX_COUNTERS], real_cyc;

  /* This is here for people that have instrumented with -lpapiex but not run under papiex -PJM */
  if (eventcnt <= 0)
    return; 

  LIBPAPIEX_DEBUG("STOP POINT %d\n",point);

  retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
      return;
    }
  thread = (papiex_perthread_data_t *)tmp;
  if (thread == NULL)
    {
      LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.");
      return;
    }

  if (!papi_mpx_zero_counts_bug || !multiplex || (get_running_usecs() > 1e6)) {
    LIBPAPIEX_DEBUG("Calling PAPI_read_ts\n");
    retval = PAPI_read_ts(thread->eventset,counters,&real_cyc);
    LIBPAPIEX_DEBUG("Returned from PAPI_read_ts\n");
    if (retval != PAPI_OK) LIBPAPIEX_PAPI_WARN("PAPI_read_ts",retval);
  }
  else {
    LIBPAPIEX_WARN("Skipped read of counters as running time too low, and PAPI_MPX_ZERO_COUNTS_BUG is enabled\n. Try running without multiplexing to avoid this bug.");
  }
  
#if 0
  real_cyc = PAPI_get_real_cyc();
  LIBPAPIEX_DEBUG("Calling PAPI_read..\n");
  retval = PAPI_read(thread->eventset,counters);
  if (retval != PAPI_OK) LIBPAPIEX_PAPI_WARN("PAPI_read",retval);
  LIBPAPIEX_DEBUG("Returned from PAPI_read..\n");
#endif

  if (point < 0) 
    {
      LIBPAPIEX_ERROR("Caliper point %d is out of range",point);
      return;
    }
  if (point >= thread->max_caliper_entries)
    {
      LIBPAPIEX_DEBUG("Caliper point %d is out of range, max %d",point,thread->max_caliper_entries);
      return;
    }
  if (thread->data[point].depth == 0)
    {
      LIBPAPIEX_ERROR("Caliper point %d is not in use",point);
      return;
    }
  thread->data[point].depth--;
  thread->data[point].used++;
  thread->data[point].real_cyc += real_cyc - thread->data[point].tmp_real_cyc;

  if (retval == PAPI_OK)
    {
      for (i=0;i<eventcnt;i++)
	thread->data[point].counters[i] += counters[i] - thread->data[point].tmp_counters[i];
#ifndef FULL_CALIPER_DATA
      if (point == 0)
#endif
	{
	  thread->data[point].real_usec += PAPI_get_real_usec() - thread->data[point].tmp_real_usec;
	  thread->data[point].virt_usec += PAPI_get_virt_usec() - thread->data[point].tmp_virt_usec;
	  thread->data[point].virt_cyc += PAPI_get_virt_cyc() - thread->data[point].tmp_virt_cyc;
          if (!no_mpi_prof && is_mpied) {
	    thread->data[point].mpiprof += thread->papiex_mpiprof - thread->data[point].tmp_mpiprof;
	    thread->data[point].mpisyncprof += thread->papiex_mpisyncprof - thread->data[point].tmp_mpisyncprof;
          }
          if (!no_io_prof)
	    thread->data[point].ioprof += thread->papiex_ioprof - thread->data[point].tmp_ioprof;
          if (!no_threadsync_prof)
	    thread->data[point].threadsyncprof += thread->papiex_threadsyncprof - thread->data[point].tmp_threadsyncprof;
	}
    }
  LIBPAPIEX_DEBUG("STOP POINT %d USED %d DEPTH %d\n",point,(int)thread->data[point].used,thread->data[point].depth);
}

void papiex_stop__(int *point)
{
  papiex_stop(*point);
}
void papiex_stop_(int *point)
{
  papiex_stop(*point);
}
void PAPIEX_STOP__(int *point)
{
  papiex_stop(*point);
}
void PAPIEX_STOP_(int *point)
{
  papiex_stop(*point);
}

void papiex_accum(int point)
{
  papiex_perthread_data_t *thread = NULL;
  void *tmp;
  int i, retval;
  long long counters[PAPIEX_MAX_COUNTERS], real_cyc;

  /* This is here for people that have instrumented with -lpapiex but not run under papiex -PJM */
  if (eventcnt <= 0)
    return; 

  LIBPAPIEX_DEBUG("ACCUM POINT %d\n",point);
  
  retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
      return;
    }
  thread = (papiex_perthread_data_t *)tmp;
  if (thread == NULL)
    {
      LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.");
      return;
    }
  
  if (!papi_mpx_zero_counts_bug || !multiplex || (get_running_usecs() > 1e6)) {
    LIBPAPIEX_DEBUG("Calling PAPI_read_ts\n");
    retval = PAPI_read_ts(thread->eventset,counters,&real_cyc);
    LIBPAPIEX_DEBUG("Returned from PAPI_read_ts\n");
    if (retval != PAPI_OK) LIBPAPIEX_PAPI_WARN("PAPI_read_ts",retval);
  }
  else {
    LIBPAPIEX_WARN("Skipped read of counters as running time too low, and PAPI_MPX_ZERO_COUNTS_BUG is enabled\n. Try running without multiplexing to avoid this bug.");
  }
#if 0
  else {
    real_cyc = PAPI_get_real_cyc();
    retval = PAPI_read(thread->eventset,counters);
    if (retval != PAPI_OK) LIBPAPIEX_PAPI_WARN("PAPI_read",retval);
  }
#endif
    
  /* Accum can't be called on point 0 */
  if (point <= 0) 
    {
      LIBPAPIEX_ERROR("Caliper point %d is out of range",point);
      return;
    }
  if (point >= thread->max_caliper_entries)
    {
      LIBPAPIEX_DEBUG("Caliper point %d is out of range, max %d",point,thread->max_caliper_entries);
      return;
    }
  if (thread->data[point].depth == 0)
    {
      LIBPAPIEX_ERROR("Caliper point %d is not in use",point);
      return;
    }
  thread->data[point].used++;

  thread->data[point].real_cyc += real_cyc - thread->data[point].tmp_real_cyc;
  thread->data[point].tmp_real_cyc = real_cyc;

  if (retval == PAPI_OK)
    {
      for (i=0;i<eventcnt;i++)
	{
	  thread->data[point].counters[i] += counters[i] - thread->data[point].tmp_counters[i];
	  thread->data[point].tmp_counters[i] = counters[i];
	}
#ifndef FULL_CALIPER_DATA
      if (point == 0)
#endif
	{
	  long long tmp = PAPI_get_real_usec();
	  thread->data[point].real_usec += tmp - thread->data[point].tmp_real_usec;
	  thread->data[point].tmp_real_usec = tmp;
	  tmp = PAPI_get_virt_usec();
	  thread->data[point].virt_usec += tmp - thread->data[point].tmp_virt_usec;
	  thread->data[point].tmp_virt_usec = tmp;
	  tmp = PAPI_get_virt_cyc();
	  thread->data[point].virt_cyc += tmp - thread->data[point].tmp_virt_cyc;
	  thread->data[point].tmp_virt_cyc = tmp;
	}
    }
  LIBPAPIEX_DEBUG("POINT %d USED %d DEPTH %d\n",point,(int) thread->data[point].used,thread->data[point].depth);
}

void papiex_accum__(int *point)
{
  papiex_accum(*point);
}
void papiex_accum_(int *point)
{
  papiex_accum(*point);
}
void PAPIEX_ACCUM__(int *point)
{
  papiex_accum(*point);
}
void PAPIEX_ACCUM_(int *point)
{
  papiex_accum(*point);
}

#ifdef HAVE_BINUTILS
static void resolve_symbols(papiex_perthread_data_t *thread)
{
  int j;
  for (j=1;j<thread->max_caliper_entries;j++)
    {
      if (thread->data[j].used >0)
	{
	  void *addr;
	  char *file, *func, t;
	  int line, num;
	  /* Pathscale */
	  if (pathscale_prof && (sscanf(thread->data[j].label,"%c%d:%p",&t,&num,&addr) == 3))
	    {
	      bfd_find_src_loc(addr,&file,&line,&func);
	      if (t == 'C')
		sprintf(thread->data[j].label,"Function %s at %p, File: %s, Function: %s, Line: %d",
			func,addr,file,func,line);
	      else
		sprintf(thread->data[j].label,"Loop %d at %p, File: %s, Function: %s, Line: %d",
			num,addr,file,func,line);
	    }
	  /* GCC */
	  if (gcc_prof && (sscanf(thread->data[j].label,":%p",&addr) == 1))
	    {
	      bfd_find_src_loc(addr,&file,&line,&func);
	      sprintf(thread->data[j].label,"Function %s at %p, File: %s, Function: %s, Line: %d",
		      func,addr,file,func,line);
	    }
	}
    }
}
#endif

static void gather_papi_thread_data_ptrs(PAPI_all_thr_spec_t *all_threads)
{

//  all_threads->num = 4096; // Fix for PAPI 3.5.0 and earlier
//  all_threads->id = (unsigned long *)malloc(all_threads->num * sizeof(unsigned long));
//  all_threads->data = (void **)malloc(all_threads->num * sizeof(void *));

#ifdef HAVE_BINUTILS
  int i;
  if (gcc_prof || pathscale_prof)
    {
      LIBPAPIEX_DEBUG("Fixing caliper labels for process %d\n", myrank);
      bfd_open_executable((char*)fullname);
      for (i=0;i<all_threads->num;i++)
	{
          LIBPAPIEX_DEBUG("\tResolving caliper labels in thread %d\n", i);
	  papiex_perthread_data_t *thread = all_threads->data[i];
	  resolve_symbols(thread);
	}
      bfd_close_executable();
    }
#endif
}

static void papiex_thread_shutdown_routine(void)
{
  void *tmp;
  papiex_perthread_data_t *thread = NULL;
  int retval;

  LIBPAPIEX_DEBUG("THREAD SHUTDOWN START\n");

  papiex_stop(0);

  retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
      return;
    }
  thread = (papiex_perthread_data_t *)tmp;
  if (thread == NULL)
    {
      LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.");
      return;
    }
  if (papi_broken_stop && multiplex && _papiex_threaded && (PAPI_thread_id() == 0)) {
    /* skip calling PAPI_stop, as it dies */
  }
  else {
    if (!papi_mpx_zero_counts_bug || !multiplex || (get_running_usecs() > 1e6)) {
      PAPI_stop(thread->eventset,NULL);
    }
    else {
      LIBPAPIEX_WARN("Skipped read of counters as running time too low, and PAPI_MPX_ZERO_COUNTS_BUG is enabled\n. Try running without multiplexing to avoid this bug.");
    }
  }
  time(&thread->finish);

#ifdef HAVE_BINUTILS
  if (gcc_prof)
    fptr_papiex_gcc_thread_shutdown(thread);
  if (pathscale_prof)
    {
      fptr_papiex_pathscale_thread_shutdown(thread);
    }
#endif

  retval = PAPI_cleanup_eventset(thread->eventset);
  if (retval != PAPI_OK) 
    LIBPAPIEX_PAPI_WARN("PAPI_cleanup_eventset",retval);

  retval = PAPI_destroy_eventset(&thread->eventset);
  if (retval != PAPI_OK) 
    LIBPAPIEX_PAPI_WARN("PAPI_destroy_eventset",retval);

  retval = PAPI_unregister_thread();
  if (retval != PAPI_OK) 
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_unregister_thread",retval);
       return;
    }

  LIBPAPIEX_DEBUG("THREAD SHUTDOWN END\n");
  return;
}

/* Only the main process thread comes to this routine, no other thread */
/* This routine gets called either from MPI_Finalize or from the process
   cleanup handler */

static void papiex_process_shutdown_routine(void)
{
  papiex_perthread_data_t process_data_sums;

  LIBPAPIEX_DEBUG("PROCESS SHUTDOWN START (rank %d)\n", myrank);

  /* for well-formed programs we should not need to do this */
  /* doing it can potentially cause loops as well */
  //if (is_mpied && !monitor_fini_mpi_callback_occurred) {
  //  LIBPAPIEX_DEBUG("monitor_fini_mpi was never called, so calling it before doing shutdown\n");
  //  monitor_fini_mpi();
  //}

  /* we may already have done a process shutdown via the monitor_fini_mpi callback */
  /* monitor_fini_mpi does a process shutdown for classic papiex + MPI combination */
  if (called_process_shutdown) return;

  called_process_shutdown = 1;
  if ((!is_mpied) && (!quiet) && _papiex_debug) print_banner(stderr);

#ifdef HAVE_BINUTILS
  if (gcc_prof)
    fptr_papiex_gcc_set_gate(0);

  if (pathscale_prof)
    fptr_papiex_pathscale_set_gate(0);
#endif

  if (!no_mpi_prof && fptr_papiex_mpiprof_set_gate)
    fptr_papiex_mpiprof_set_gate(0);

  if (!no_io_prof && fptr_papiex_ioprof_set_gate)
    fptr_papiex_ioprof_set_gate(0);

  if (!no_threadsync_prof && fptr_papiex_threadsyncprof_set_gate)
    fptr_papiex_threadsyncprof_set_gate(0);

  /* Set process finish time if it's not set */
  if (proc_fini_time.tv_sec == 0)
    gettimeofday(&proc_fini_time, NULL);

  /* we invoke the thread shutdown here so we have this
   * thread stats as well, when we go to compute the 
   * process stats. Note, this means we MUST have a 
   * monitor_real_exit/monitor_real_exec at the end of
   * this function to make sure the automatic thread
   * shutdown callback does not cause the thread_shutdown
   * to run again. */
  LIBPAPIEX_DEBUG("Invoking thread shutdown in main thread (rank %d)\n", myrank);
  papiex_thread_shutdown_routine();
      
  gather_papi_thread_data_ptrs(&all_threads);
  LIBPAPIEX_DEBUG("Rank (%d): Num. of threads = %d\n", myrank, all_threads.num);
    
  /* Create the needed directories */ 
  get_base_output_path(user_path_prefix, output_file_name);
  if (!no_write) make_output_paths(_papiex_threaded);

  LIBPAPIEX_DEBUG("START print_process_stats rank %d\n",myrank);
  print_process_stats(&all_threads, &process_data_sums, stderr);
  LIBPAPIEX_DEBUG("FINISHED print_process_stats rank %d\n",myrank);

  // launch postprocessing script for papiex_nextgen papiex and exit
  // This script must be launched exactly once for the whole job
  // get_rank() returns 0 for MPI rank 0, and for non-MPI programs
  if (papiex_nextgen && (myrank==0)) {

      // wait until all the other proceses in this MPI job have finished
      // writing their files before aggregate stats
      if (ntasks > 1) {
        int i;
        char placeholder[PATH_MAX];
        LIBPAPIEX_DEBUG("Master waiting for all child processes to finish writing their files.\n");
        LIBPAPIEX_DEBUG("Base output path: %s\n", base_output_path);
        for (i=0; i<ntasks; i++) {
          sprintf(placeholder, "%s/.rank.%d.complete",base_output_path, i);
          while (!file_exists(placeholder)) { sleep(1); }
          unlink(placeholder); // remove file once we have found it
          LIBPAPIEX_DEBUG("Rank %d done\n", i);
        }
        unlink(path_container);
        LIBPAPIEX_DEBUG("Master done waiting. All child processes are done.\n");
      }

      /* set the following environment variables so the report generation
       * phase can figure out the wallclock timings of start, finish and duration
       */
      char wallclock_start[256];
      char wallclock_finish[256];
      char wallclock_usecs[256];
      snprintf(wallclock_start, 256, "%s", ctime(&proc_init_time.tv_sec));
      snprintf(wallclock_finish, 256, "%s", ctime(&proc_fini_time.tv_sec));
      snprintf(wallclock_usecs, 256, "%f", (float)((1000000 * proc_fini_time.tv_sec + proc_fini_time.tv_usec) - (1000000 * proc_init_time.tv_sec+proc_init_time.tv_usec)));
      setenv("PAPIEX_WALLCLOCK_START", wallclock_start, 1);
      setenv("PAPIEX_WALLCLOCK_FINISH", wallclock_finish, 1);
      setenv("PAPIEX_WALLCLOCK_USECS", wallclock_usecs, 1);

      char shell_cmd[PATH_MAX];
      /* we either run the Python script papiex-output2csv OR
       * the Perl papiex-report script */
      if (csv_output) {
          sprintf(shell_cmd, "papiex-output2csv %s", is_mpied? get_base_output_path(user_path_prefix, output_file_name):proc_stats_path);
      }
      else {
          char *papiex_report_args = getenv("PAPIEX_REPORT_ARGS");
          if (!papiex_report_args) papiex_report_args = "";
          char prefix_args[PATH_MAX] = "";
          if (strlen(user_path_prefix)) sprintf(prefix_args, "-prefix=%s", user_path_prefix);
          sprintf(shell_cmd, "papiex-report %s %s %s -spec=%s %s %s", (_papiex_debug? "-d":""), (user_specified_spec? "-nosearch": ""), prefix_args, get_spec_file(), papiex_report_args, is_mpied? get_base_output_path(user_path_prefix, output_file_name):proc_stats_path);
          char *arch_values = getenv("PAPIEX_REPORT_ARCH_VALUES");
          if (!arch_values) arch_values = "";
          else LIBPAPIEX_DEBUG("\nSetting PAPIEX_REPORT_ARCH_VALUES=%s\n", arch_values);
          setenv("PAPIEX_REPORT_ARCH_VALUES", arch_values, 1);
      }
      LIBPAPIEX_DEBUG("\nExecuting: /bin/sh -c \"%s\"\n", shell_cmd);
      if (monitor_real_system(shell_cmd) != 0) {
          fprintf(stderr, "Error running %s\n", shell_cmd);
          monitor_real_exit (1);
      }
  }

  const char* exec_cmd = getenv("PAPIEX_EXEC_CMD");
  if (exec_cmd) {
    LIBPAPIEX_DEBUG("Running %s \n", exec_cmd);
    monitor_real_system(exec_cmd);
  }
  LIBPAPIEX_DEBUG("PROCESS SHUTDOWN END (rank %d)\n", myrank);
}

void papiex_start(int point, char *label)
{
  int retval;
  papiex_perthread_data_t *thread = NULL;
  void *tmp;

  /* This is here for people that have instrumented with -lpapiex but not run under papiex -PJM */
  if (eventcnt <= 0)
    return; 

  LIBPAPIEX_DEBUG("START POINT %d LABEL %s\n",point,label);

  retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
      return;
    }
  thread = (papiex_perthread_data_t *)tmp;
  if (thread == NULL)
    {
      LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.");
      return;
    }
  if (point < 0)
    {
      LIBPAPIEX_ERROR("Caliper point %d is out of range",point);
      return;
    }
  if (point >= thread->max_caliper_entries)
    {
      LIBPAPIEX_DEBUG("Caliper point %d is out of range, max %d",point,thread->max_caliper_entries);
      return;
    }
  if (thread->data[point].depth)
    {
      LIBPAPIEX_ERROR("Caliper point %d is already in use",point);
      return;
    }
  thread->data[point].depth++;

  if (label && point && (thread->data[point].label[0] == '\0'))
    {
      if (strlen(label) > MAX_LABEL_SIZE-1) {
	PAPIEX_WARN("Max. label size is %d bytes, ignoring some characters of %s\n", MAX_LABEL_SIZE-1, label);
      }
      strncpy(thread->data[point].label, label, MAX_LABEL_SIZE-1);
      thread->data[point].label[MAX_LABEL_SIZE-1] = '\0';
    }

  /* Update the max caliper point used */
  if (thread->max_caliper_used <= point)
    thread->max_caliper_used = point;

#ifndef FULL_CALIPER_DATA
  if (point == 0)
#endif
    {
      thread->data[point].tmp_virt_usec = PAPI_get_virt_usec();
      thread->data[point].tmp_real_usec = PAPI_get_real_usec();
      thread->data[point].tmp_virt_cyc = PAPI_get_virt_cyc();
      if (!no_mpi_prof && is_mpied) {
        thread->data[point].tmp_mpiprof = thread->papiex_mpiprof;
        thread->data[point].tmp_mpisyncprof = thread->papiex_mpisyncprof;
      }
      if (!no_io_prof)
        thread->data[point].tmp_ioprof = thread->papiex_ioprof;
      if (!no_threadsync_prof)
        thread->data[point].tmp_threadsyncprof = thread->papiex_threadsyncprof;
    }

  /* There is a bug in PAPI, so if we call PAPI_read right after the intial
   * PAPI_start, then we get a FP exception in the PAPI_read because the 
   * the time of running was zero.
   * We work around the bug by making sure we only call PAPI_read after atleast
   * 1sec of real time. Otherwise, it's assumed that the counters are still 
   * at the intial value of zero.
   */
  if (!papi_mpx_zero_counts_bug || !multiplex || (get_running_usecs() > 1e6)) {
    LIBPAPIEX_DEBUG("Calling PAPI_read_ts\n");
    retval = PAPI_read_ts(thread->eventset,thread->data[point].tmp_counters,&thread->data[point].tmp_real_cyc);
    LIBPAPIEX_DEBUG("Returned from PAPI_read_ts\n");
    if (retval != PAPI_OK) LIBPAPIEX_PAPI_WARN("PAPI_read_ts",retval);
#if 0
    else {
      thread->data[point].tmp_real_cyc = PAPI_get_real_cyc();
      LIBPAPIEX_DEBUG("Reading counters (PAPI_read) to get initial counts\n");
      retval = PAPI_read(thread->eventset,thread->data[point].tmp_counters);
      if (retval != PAPI_OK) LIBPAPIEX_PAPI_WARN("PAPI_read",retval);
    }
#endif
  }
  else {
    LIBPAPIEX_DEBUG("Skipped read of counters as running time too low, and PAPI_MPX_ZERO_COUNTS_BUG is enabled\n. Try running without multiplexing to avoid this bug.");
  }

  LIBPAPIEX_DEBUG("START POINT %d USED %d DEPTH %d\n",point,(int) thread->data[point].used,thread->data[point].depth);
}

void papiex_start__(int *point, char *label)
{
  papiex_start(*point, label);
}
void papiex_start_(int *point, char *label)
{
  papiex_start(*point, label);
}
void PAPIEX_START__(int *point, char *label)
{
  papiex_start(*point, label);
}
void PAPIEX_START_(int *point, char *label)
{
  papiex_start(*point, label);
}

static void papiex_thread_init_routine(void)
{
#ifdef HAVE_PAPI
  int retval, eventset;
  papiex_perthread_data_t *thread;
#endif

#ifdef HAVE_PAPI
  LIBPAPIEX_DEBUG("THREAD INIT START malloc(%lu)\n",(long unsigned int)sizeof(papiex_perthread_data_t));
  thread = malloc(sizeof(papiex_perthread_data_t));
  if (thread == NULL)
    {
      LIBPAPIEX_ERROR("malloc(%d) for per thread data failed.",(int)sizeof(papiex_perthread_data_t));
      monitor_real_exit(1);
    }
  /* The thread data structure is initialized to zero */
  memset(thread,0x0,sizeof(papiex_perthread_data_t));
  thread->max_caliper_entries = PAPIEX_MAX_CALIPERS;

  eventset = PAPI_NULL;
  retval = PAPI_create_eventset(&eventset);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_create_eventset",retval);
      return;
    }

  if (multiplex)
    {
      retval = PAPI_assign_eventset_component(eventset, 0);
      if (retval != PAPI_OK) {
        LIBPAPIEX_PAPI_ERROR("PAPI_assign_eventset_component",retval);
        return;
      }

      //if (mpx_interval > 0) {
      if (mpx_interval != PAPIEX_DEFAULT_MPX_HZ) {
        LIBPAPIEX_DEBUG("PAPI multiplex interval set to: %d Hz\n", mpx_interval);
        PAPI_multiplex_option_t popt;
	memset(&popt,0,sizeof(popt));
#ifdef PAPI_DEF_ITIMER_NS
        popt.ns = (int)(1000000000 / mpx_interval);
        retval = PAPI_set_opt(PAPI_DEF_ITIMER_NS, (PAPI_option_t *)&popt);
#else
        popt.us = (int)(1000000 / mpx_interval);
        retval = PAPI_set_opt(PAPI_DEF_MPX_USEC, (PAPI_option_t *)&popt);
#endif
        if (retval != PAPI_OK)
  	{
  	  LIBPAPIEX_PAPI_ERROR("PAPI_set_opt (multiplex interval)",retval);
  	  return;
  	}
      } // mpx_interval

      LIBPAPIEX_DEBUG("Calling PAPI_set_multiplex on the eventset\n");
      retval = PAPI_set_multiplex(eventset);
      if (retval != PAPI_OK)
      	{
	  LIBPAPIEX_PAPI_ERROR("PAPI_set_multiplex",retval);
	  return;
	}
    }

  if (eventcnt == 0) {
    eventcodes[0] = PAPI_TOT_CYC;
    eventnames[0] = strdup("PAPI_TOT_CYC");
    if (PAPI_query_event(PAPI_FP_OPS) == PAPI_OK) {
      eventcodes[1] = PAPI_FP_OPS;
      eventnames[1] = strdup("PAPI_FP_OPS");
    }
    else if (PAPI_query_event(PAPI_FP_INS) == PAPI_OK) {
      eventcodes[1] = PAPI_FP_INS;
      eventnames[1] = strdup("PAPI_FP_INS");
    }
    else {
      eventcodes[1] = PAPI_TOT_INS;
      eventnames[1] = strdup("PAPI_TOT_INS");
    }
    eventcnt = 2;
  }

  int i;
  PAPI_lock(PAPI_LOCK_USR2);
  for (i=0; i<eventcnt; i++) {
    LIBPAPIEX_DEBUG("Calling PAPI_add_event(%d,0x%x) (%s)\n", eventset,eventcodes[i],eventnames[i]);
    retval = PAPI_add_event(eventset, eventcodes[i]);
    if (retval < PAPI_OK) {
      LIBPAPIEX_DEBUG("PAPI_add_event(%d,0x%x) (%s)\n",eventset,eventcodes[i],eventnames[i]);
      LIBPAPIEX_PAPI_ERROR("PAPI_add_events",retval);
      PAPI_unlock(PAPI_LOCK_USR2);
      return;
    }
  }
  PAPI_unlock(PAPI_LOCK_USR2);
  
  if (!spec_file) spec_file = DEFAULT_SPEC_FILE;
  
  thread->eventset = eventset;
  retval = PAPI_set_thr_specific(PAPI_TLS_USER_LEVEL1,thread);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_set_thr_specific",retval);
      return;
    }

  if (multiplex && papi_mpx_zero_counts_bug)
    init_virt_cyc = PAPI_get_virt_cyc();

  LIBPAPIEX_DEBUG("Starting counters with PAPI_start\n");
  retval = PAPI_start(thread->eventset);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_start",retval);
      return;
    }

  time(&thread->stamp);
#endif

#ifdef HAVE_BINUTILS
  if (gcc_prof && fptr_papiex_gcc_thread_init)
    fptr_papiex_gcc_thread_init(thread);

  if (pathscale_prof && fptr_papiex_pathscale_thread_init)
    fptr_papiex_pathscale_thread_init(thread);
#endif 

  LIBPAPIEX_DEBUG("Calling PAPI_lock before critical section\n");
  PAPI_lock(PAPI_LOCK_USR2);
  if (all_threads.num+1 >= all_threads_size)
    {
      all_threads.id = (PAPI_thread_id_t *)realloc(all_threads.id,(all_threads_size*2*sizeof(PAPI_thread_id_t)));
      all_threads.data = (void **)realloc(all_threads.data,(all_threads_size*2*sizeof(void *)));
      if ((all_threads.data == NULL) || (all_threads.id == NULL)) {
	LIBPAPIEX_ERROR("malloc() for %d thread entries failed.",all_threads_size*2);
	monitor_real_exit(1);
      }
      all_threads_size = all_threads_size*2;
    }
  //all_threads.id[all_threads.num] = syscall(SYS_gettid);
  all_threads.id[all_threads.num] = monitor_get_thread_num();
  all_threads.data[all_threads.num] = thread;
  all_threads.num++;

  PAPI_unlock(PAPI_LOCK_USR2);
  LIBPAPIEX_DEBUG("Released PAPI lock\n");

  papiex_start(0,"");

  LIBPAPIEX_DEBUG("THREAD INIT END\n");
}

#if defined(__GNUC__) && !defined(HAVE_MONITOR)
static void papiex_process_init_routine() __attribute__((constructor));
#elif defined(sun)
#pragma init (papiex_process_init_routine)
#endif

static void papiex_process_init_routine(void)
{
  int retval, i;
  int version;

  /* remove stale files if around */
  unlink(path_container);

  /* after a fork, this better be initialized to 0 in the child
   * Otherwise, we will end up intercepting I/O calls and calling PAPI_get_real_usec before
   * PAPI_library_init
   */
  LIBPAPIEX_DEBUG("PROCESS INIT START\n");

  if (getenv("PAPIEX_PAPI_MPX_ZERO_COUNTS_BUG")) {
    papi_mpx_zero_counts_bug = 1;
    LIBPAPIEX_DEBUG("Will workaround PAPI's multiplex zero counts bug if needed\n");
  }
  if (getenv("PAPIEX_BROKEN_PAPI_STOP")) {
    papi_broken_stop = 1;
    LIBPAPIEX_DEBUG("Broken PAPI_stop for multiplex+pthreads. Will work around it.\n");
  }

  LIBPAPIEX_DEBUG("Initializing the PAPI library\n");
  version = PAPI_library_init(PAPI_VER_CURRENT);
  if (version != PAPI_VER_CURRENT)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_library_init",version);
      return;
    }


  if ((hwinfo = PAPI_get_hardware_info()) == NULL) 
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_get_hardware_info returned NULL",0);
      return;
    }
  processor = hwinfo->model_string;
  clockrate = hwinfo->mhz;
  if (clockrate <= 0) clockrate = hwinfo->cpu_max_mhz;

  if ((exeinfo = PAPI_get_executable_info()) == NULL)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_get_executable_info",0);
      return;
    }
  process_name = (char *)exeinfo->address_info.name;
  fullname = exeinfo->fullname;

  for (i = 0;i<eventcnt;i++)
    {
      int eventcode = 0;
      LIBPAPIEX_DEBUG("Checking event %s for existence.\n",eventnames[i]);
      retval = PAPI_event_name_to_code(eventnames[i],&eventcode);
      if (retval != PAPI_OK)
	{
          LIBPAPIEX_WARN("Could not map name to code for %s\n", eventnames[i]);
	  LIBPAPIEX_PAPI_ERROR("PAPI_event_name_to_code",retval);
	  return;
	}
      eventcodes[i] = eventcode;
    }

#if 0
  if (no_follow_fork)
    {
      /* Here we should just delete the string, not unsetenv it. */
      PAPI_option_t pl;
      PAPI_get_opt(PAPI_PRELOAD, &pl);
      unsetenv(pl.preload.lib_preload_env);
    }
#endif

  if (multiplex)
    {
      LIBPAPIEX_DEBUG("Enabling multiplexing in the PAPI library\n");
      retval = PAPI_multiplex_init();
      if (retval != PAPI_OK)
	{
	  LIBPAPIEX_PAPI_ERROR("PAPI_multiplex_init",retval);
	  return;
	}
    }

  if (domain == 0)
    domain = PAPI_DOM_USER;

  LIBPAPIEX_DEBUG("Setting PAPI domain\n");
  retval = PAPI_set_domain(domain);
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_set_domain",retval);
      return;
    }

  if (!no_mpi_prof && fptr_papiex_mpiprof_set_gate) {
    fptr_papiex_mpiprof_set_gate(1);
  }

  if (!no_io_prof && fptr_papiex_ioprof_set_gate) {
    fptr_papiex_ioprof_set_gate(1);
  }

  if (!no_threadsync_prof && fptr_papiex_threadsyncprof_set_gate) {
    fptr_papiex_threadsyncprof_set_gate(1);
  }

#ifdef HAVE_BINUTILS
  if (gcc_prof && fptr_papiex_gcc_set_gate) {
    fptr_papiex_gcc_set_gate(gcc_prof);
  }

  if (pathscale_prof && fptr_papiex_pathscale_set_gate) {
    fptr_papiex_pathscale_set_gate(pathscale_prof);
  }
#endif

  all_threads.id = (PAPI_thread_id_t *)realloc(all_threads.id,(all_threads_size*sizeof(PAPI_thread_id_t)));
  all_threads.data = (void **)realloc(all_threads.data,(all_threads_size*sizeof(void *)));
  if ((all_threads.data == NULL) || (all_threads.id == NULL)) {
    LIBPAPIEX_ERROR("malloc() for %d thread entries failed.",all_threads_size);
    monitor_real_exit(1);
  }

  //if ((!is_mpied) && (!quiet)) print_banner(stderr);

  gettimeofday(&proc_init_time, NULL);

  papiex_thread_init_routine();

  LIBPAPIEX_DEBUG("PROCESS INIT END\n");
}

static inline int get_num_tasks(void) {
  if (is_mpied && (ntasks==1)) {
    ntasks = monitor_mpi_comm_size();
  }
  return ntasks;
}


extern void monitor_init_mpi(int *argc, char ***argv) {
  LIBPAPIEX_DEBUG("monitor_init_mpi callback invoked\n");
  is_mpied = 1;
  return;
}

extern void monitor_fini_mpi(void) {
  LIBPAPIEX_DEBUG("monitor_fini_mpi callback invoked\n");
  monitor_fini_mpi_callback_occurred = 1;
  ntasks = monitor_mpi_comm_size();
  myrank = monitor_mpi_comm_rank();
  LIBPAPIEX_DEBUG("rank=%d, size=%d\n", myrank, ntasks);
  if ((myrank == 0) && (!quiet) && _papiex_debug) print_banner(stderr);
  if (myrank == 0) {
    set_base_output_path(user_path_prefix, output_file_name);
  }
  /* Force monitor to call the fini_process callback, monitor will guarantee 
     us that this only happens once. Only do this if we need to use MPI
     to gather the data. As we only do MPI gather in the classic mode
     of papiex and that too only if no_mpi_gather is unset, the code below
     is invoked in rare exceptions.
     The reason we invoke the fini process handler here for the classic
     papiex is that we need to do an MPI gather before the real MPI finalize
     is called after this callback. */
  if (!papiex_nextgen && !no_mpi_gather) {
    monitor_fini_process(1, NULL);
  }
}

static void print_process_stats(PAPI_all_thr_spec_t *process, 
                                papiex_perthread_data_t *process_data_sums,
                                FILE *output) 
{
  struct hsearch_data caliper_label_to_index_hash;
  int i, j, k;
  //unsigned long tid;
  papiex_perthread_data_t *thr_data;
  papiex_perthread_data_t mean_d, min_d, max_d, stddev_d;
  papiex_perthread_data_t* mean_data = &mean_d;
  papiex_perthread_data_t* min_data = &min_d;
  papiex_perthread_data_t* max_data = &max_d;
  papiex_perthread_data_t* stddev_data = &stddev_d;
  papiex_double_perthread_data_t squared_sums;
  int rank = myrank;

  assert(process);
  assert(process_data_sums);

  memset(mean_data, 0, sizeof(papiex_perthread_data_t));
  memset(stddev_data, 0, sizeof(papiex_perthread_data_t));
  memset(min_data, 0, sizeof(papiex_perthread_data_t));
  memset(max_data, 0, sizeof(papiex_perthread_data_t));
  memset(&squared_sums, 0, sizeof(papiex_double_perthread_data_t));
  memset(process_data_sums, 0, sizeof(papiex_perthread_data_t));

  // This is here to make the top-level not have to calls with different arguments
  // not ideal, I agree
  if ((output == stderr) && write_only) output = NULL;

  nthreads = process->num;
  LIBPAPIEX_DEBUG("Computing stats for process rank=%d, %d threads, process_ptr=%p\n",
                   rank, nthreads, process);

  strcpy(proc_stats_path, get_proc_output_path());
  if (! _papiex_threaded) strcat(proc_stats_path, suffix);
  LIBPAPIEX_DEBUG("Process output path: %s\n", proc_stats_path);

  if (!quiet && !no_write &&  (output != NULL) && !is_mpied && _papiex_debug)
    fprintf(output, "\n%s: Storing raw counts data in [%s]\n", 
                     tool, proc_stats_path);

  float process_walltime_usec = (1000000 * proc_fini_time.tv_sec + proc_fini_time.tv_usec) -
                                 (1000000 * proc_init_time.tv_sec + proc_init_time.tv_usec);

  int scope = (_papiex_threaded ? THR_SCOPE:PROC_SCOPE);
  for (i=0;i<nthreads;i++) {
    //tid = process->id[i];
    thr_data = process->data[i];

    /* do we need to write, per-thread-data?*/
    if (!no_write) {
      char thr_file[PATH_MAX];

      /* If it's a threaded program then we use the proc_stats_path
       * to derive the file name, by appending the thread id.
       * If it's an unthreaded program, then we just use 
       * set the thread file name to the proc_stats_path
       */
      if (_papiex_threaded)
        strcpy(thr_file, get_thread_output_path(i));
      else
        strcpy(thr_file, proc_stats_path);

      unlink(thr_file);
      FILE *thr_out = fopen(thr_file, "w");
      if (!thr_out) {
        LIBPAPIEX_ERROR("fopen(%s) failed. %s",
                thr_file, strerror(errno));
        monitor_real_exit(1);
      }
      /* one hack: */
      /* For the main thread in an MPI threaded program, the
       * finish time is not set. We set it here
       */
      if (thr_data->finish == 0)
        thr_data->finish = proc_fini_time.tv_sec;

      print_thread_stats(thr_out, thr_data, i, scope, process_walltime_usec);
      fclose(thr_out);
      LIBPAPIEX_DEBUG("Finished printing stats for thread %d\n", i);
    }
  }
  /* finished writing thread output into thr-specific file */
  if (papiex_nextgen && (ntasks > 1)) {
    char placeholder[PATH_MAX];
    sprintf(placeholder, "%s/.rank.%d.complete",get_base_output_path(user_path_prefix, output_file_name), myrank);
    FILE* ph = fopen(placeholder, "w");
    LIBPAPIEX_DEBUG("Created placeholder file (%s) to signal my work is done\n", placeholder);
    fclose(ph);
  }

  /* if this is papiex_nextgen papiex, all post-processing is done in a script */
  if (papiex_nextgen) return;

//#warning "Why is this here? This should be done just like every other program, not modal. Can't we do this in the standard path?"

  /* If this is an unthreaded (non-MPI) program, then we additionally print the stats
   * to output and copy the thread structure to the sums */

  /* These lines may be the source of bug 3880 */

  if (!_papiex_threaded) {
    if ((output != NULL) && !is_mpied)
      print_thread_stats(output, process->data[0], 0, PROC_SCOPE,  process_walltime_usec);
    memcpy(process_data_sums, process->data[0], sizeof(papiex_perthread_data_t));
    return;
  }

  /* Return now, if the user doesn't want to do any processing */
  if (no_summary_stats) return;
  
  LIBPAPIEX_DEBUG("Computing summary stats for rank %d\n", rank);

  /* Initialize hash table to do label -> caliper index matching */
  memset(&caliper_label_to_index_hash,0x0,sizeof(caliper_label_to_index_hash));
  if (hcreate_r(PAPIEX_MAX_CALIPERS, &caliper_label_to_index_hash) == 0) 
    LIBPAPIEX_ERROR("Could not initialize hash table for caliper label to index matching\n");

  /* Compute mean and stddev over the threads */
  LIBPAPIEX_DEBUG("\tNow computing process sums stats for rank %d\n", rank);
  //printf("START sum rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  for (i=0;i<nthreads;i++) {
    //tid = process->id[i];
    thr_data = process->data[i];
    LIBPAPIEX_DEBUG("\t\tProcessing totals for thread %d\n", i);
 
    /* aggregate into the global sums data structure */
    process_data_sums->data[0].used += thr_data->data[0].used;
    process_data_sums->data[0].real_usec += thr_data->data[0].real_usec;
    process_data_sums->data[0].real_cyc += thr_data->data[0].real_cyc;
    process_data_sums->data[0].virt_usec += thr_data->data[0].virt_usec;
    process_data_sums->data[0].virt_cyc += thr_data->data[0].virt_cyc;
    if (!no_mpi_prof && is_mpied) {
      process_data_sums->data[0].mpiprof += thr_data->data[0].mpiprof;
      process_data_sums->papiex_mpiprof += thr_data->papiex_mpiprof;
      process_data_sums->data[0].mpisyncprof += thr_data->data[0].mpisyncprof;
      process_data_sums->papiex_mpisyncprof += thr_data->papiex_mpisyncprof;
    }
    if (!no_io_prof) {
      process_data_sums->data[0].ioprof += thr_data->data[0].ioprof;
      process_data_sums->papiex_ioprof += thr_data->papiex_ioprof;
    }
    if (!no_threadsync_prof) {
      process_data_sums->data[0].threadsyncprof += thr_data->data[0].threadsyncprof;
      process_data_sums->papiex_threadsyncprof += thr_data->papiex_threadsyncprof;
    }
    for (k=0;k<eventcnt;k++)
      process_data_sums->data[0].counters[k] += thr_data->data[0].counters[k];
    process_data_sums->max_caliper_entries = max (process_data_sums->max_caliper_entries, thr_data->max_caliper_entries);
    process_data_sums->max_caliper_used = max (process_data_sums->max_caliper_used, thr_data->max_caliper_used);

    /* I think this can be improved a lot. 

    * We should only process the calipers we've used
    * We should not use a hash, we should just make sure the mapping is identical for the calipers, 
      otherwise the hash has to be done everywhere, not just here. */

    for (j=1;j<thr_data->max_caliper_entries;j++) {
      if (thr_data->data[j].used >0) {

        /* We have decided not to use a caliper merge this way */
	/*
        int idx = get_label_index(thr_data->data[j].label,&caliper_label_to_index_hash);
        assert(idx>0);
        if (process_data_sums->data[idx].label[0] == '\0') {
          strcpy(process_data_sums->data[idx].label, thr_data->data[j].label);
        } 
        // This is just a sanity check
        if (strcmp(process_data_sums->data[idx].label, thr_data->data[j].label)!=0) {
          LIBPAPIEX_WARN("Something is fishy, the process label for idx=%d is %s, while the label for thread %d caliper %d is %s\n",
                          idx, process_data_sums->data[idx].label, i, j, thr_data->data[j].label);
        }
        LIBPAPIEX_DEBUG("\t\t\tProcessing caliper %d (%s)\n", j, process_data_sums->data[idx].label);
        process_data_sums->data[idx].used += thr_data->data[j].used;
	process_data_sums->data[idx].real_cyc += thr_data->data[j].real_cyc;
#ifdef FULL_CALIPER_DATA
	process_data_sums->data[idx].virt_usec += thr_data->data[j].virt_usec;
	process_data_sums->data[idx].virt_cyc += thr_data->data[j].virt_cyc;
        process_data_sums->data[idx].real_usec += thr_data->data[j].real_usec;
#endif
	for (k=0;k<eventcnt;k++)
          process_data_sums->data[idx].counters[k] += thr_data->data[j].counters[k];
	*/

        // punt if the caliper labels don't match
        if (match_caliper_label(process_data_sums->data[j].label, thr_data->data[j].label) != 0) {
	   LIBPAPIEX_WARN("Label for caliper %d is %s in the sum AND %s in thread %d\nSkipping the latter in the summation.\n"
	                  "Please treat all data (min,max,dev) for these calipers as suspect\n",
	     j, process_data_sums->data[j].label, thr_data->data[j].label, i);
           continue;
	}

        LIBPAPIEX_DEBUG("\t\t\tProcessing caliper %d (%s)\n", j, process_data_sums->data[j].label);
        process_data_sums->data[j].used += thr_data->data[j].used;
	process_data_sums->data[j].real_cyc += thr_data->data[j].real_cyc;
#ifdef FULL_CALIPER_DATA
	process_data_sums->data[j].virt_usec += thr_data->data[j].virt_usec;
	process_data_sums->data[j].virt_cyc += thr_data->data[j].virt_cyc;
        process_data_sums->data[j].real_usec += thr_data->data[j].real_usec;
        if (!no_mpi_prof && is_mpied) {
          process_data_sums->data[j].mpiprof += thr_data->data[j].mpiprof;
          process_data_sums->data[j].mpisyncprof += thr_data->data[j].mpisyncprof;
        }
        if (!no_io_prof)
          process_data_sums->data[j].ioprof += thr_data->data[j].ioprof;
        if (!no_threadsync_prof)
          process_data_sums->data[j].threadsyncprof += thr_data->data[j].threadsyncprof;
#endif
	for (k=0;k<eventcnt;k++)
          process_data_sums->data[j].counters[k] += thr_data->data[j].counters[k];
      }
    } /* calipers */
  } /* threads */

  /* remove label hash */
  hdestroy_r(&caliper_label_to_index_hash);

//  printf("START mean rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  /* divide aggregate data by nthreads to get mean */
  mean_data->data[0].used = process_data_sums->data[0].used / nthreads ; 
  mean_data->data[0].real_usec = process_data_sums->data[0].real_usec / nthreads ; 
  mean_data->data[0].real_cyc = process_data_sums->data[0].real_cyc / nthreads ; 
  mean_data->data[0].virt_usec = process_data_sums->data[0].virt_usec / nthreads ; 
  mean_data->data[0].virt_cyc = process_data_sums->data[0].virt_cyc / nthreads ; 
  if (!no_mpi_prof && is_mpied) {
    mean_data->data[0].mpiprof = process_data_sums->data[0].mpiprof / nthreads ; 
    mean_data->papiex_mpiprof = process_data_sums->papiex_mpiprof / nthreads ; 
    mean_data->data[0].mpisyncprof = process_data_sums->data[0].mpisyncprof / nthreads ; 
    mean_data->papiex_mpisyncprof = process_data_sums->papiex_mpisyncprof / nthreads ; 
  }
  if (!no_io_prof) {
    mean_data->data[0].ioprof = process_data_sums->data[0].ioprof / nthreads ; 
    mean_data->papiex_ioprof = process_data_sums->papiex_ioprof / nthreads ; 
  }
  if (!no_threadsync_prof) {
    mean_data->data[0].threadsyncprof = process_data_sums->data[0].threadsyncprof / nthreads ; 
    mean_data->papiex_threadsyncprof = process_data_sums->papiex_threadsyncprof / nthreads ; 
  }
  for (k=0;k<eventcnt;k++)
    mean_data->data[0].counters[k] = process_data_sums->data[0].counters[k] / nthreads;
  for (j=1;j<PAPIEX_MAX_CALIPERS; j++) {
    if (process_data_sums->data[j].used >0) {
//#warning "This line makes no sense to me, shouldn't this really be the mean?"
      mean_data->data[j].used = process_data_sums->data[j].used / nthreads ; 
//      mean_data->data[j].used = process_data_sums->data[j].used;
      if (strlen(process_data_sums->data[j].label) == 0)
        strcpy(mean_data->data[j].label, process_data_sums->data[j].label);
     // printf("Mean valued of used for caliper %s is %g\n", mean_data->data[j].label, mean_data->data[j].used);
      mean_data->data[j].real_cyc = process_data_sums->data[j].real_cyc / nthreads ; 
#ifdef FULL_CALIPER_DATA
      mean_data->data[j].real_usec = process_data_sums->data[j].real_usec / nthreads ; 
      mean_data->data[j].virt_usec = process_data_sums->data[j].virt_usec / nthreads ; 
      mean_data->data[j].virt_cyc = process_data_sums->data[j].virt_cyc / nthreads ; 
      if (!no_mpi_prof && is_mpied) {
        mean_data->data[j].mpiprof = process_data_sums->data[j].mpiprof / nthreads ; 
        mean_data->data[j].mpisyncprof = process_data_sums->data[j].mpisyncprof / nthreads ; 
      }
      if (!no_io_prof)
        mean_data->data[j].ioprof = process_data_sums->data[j].ioprof / nthreads ; 
      if (!no_threadsync_prof) 
        mean_data->data[j].threadsyncprof = process_data_sums->data[j].threadsyncprof / nthreads ; 
#endif
      for (k=0;k<eventcnt;k++)
        mean_data->data[j].counters[k] = process_data_sums->data[j].counters[k] / nthreads;
    }
  } /* calipers */
  //printf("START min rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  /* min computing */
  LIBPAPIEX_DEBUG("\tComputing thread min stats for rank %d\n", rank);
  thr_data = process->data[0];
  min_data->data[0].real_usec = thr_data->data[0].real_usec;
  min_data->data[0].real_cyc = thr_data->data[0].real_cyc;
  min_data->data[0].virt_usec = thr_data->data[0].virt_usec;
  min_data->data[0].virt_cyc = thr_data->data[0].virt_cyc;
  if (!no_mpi_prof && is_mpied) {
    min_data->papiex_mpiprof = thr_data->papiex_mpiprof;
    min_data->papiex_mpisyncprof = thr_data->papiex_mpisyncprof;
  }  if (!no_io_prof)
    min_data->papiex_ioprof = thr_data->papiex_ioprof;
  if (!no_threadsync_prof)
    min_data->papiex_threadsyncprof = thr_data->papiex_threadsyncprof;
  for (i=0;i<eventcnt;i++)
    min_data->data[0].counters[i] = thr_data->data[0].counters[i];
  for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
    if (thr_data->data[j].used >0) {
      min_data->data[j].used = thr_data->data[j].used;
      min_data->data[j].real_cyc = thr_data->data[j].real_cyc;
#ifdef FULL_CALIPER_DATA
      min_data->data[j].real_usec = thr_data->data[j].real_usec;
      min_data->data[j].virt_usec = thr_data->data[j].virt_usec;
      min_data->data[j].virt_cyc = thr_data->data[j].virt_cyc;
#endif
      for (k=0;k<eventcnt;k++)
        min_data->data[j].counters[k] = thr_data->data[j].counters[k];
    }
  } /* calipers */

  //printf("START min rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  for (i=1; i<nthreads; i++) {
    thr_data = process->data[i];
    LIBPAPIEX_DEBUG("\t\tProcessing thread %d for rank %d\n", i, rank);
    min_data->data[0].real_usec = min(min_data->data[0].real_usec, thr_data->data[0].real_usec);
    min_data->data[0].real_cyc = min(min_data->data[0].real_cyc, thr_data->data[0].real_cyc);
    min_data->data[0].virt_usec = min(min_data->data[0].virt_usec, thr_data->data[0].virt_usec);
    min_data->data[0].virt_cyc = min(min_data->data[0].virt_cyc, thr_data->data[0].virt_cyc);
    if (!no_mpi_prof && is_mpied) {
      min_data->papiex_mpiprof = min(min_data->papiex_mpiprof, thr_data->papiex_mpiprof);
      min_data->papiex_mpisyncprof = min(min_data->papiex_mpisyncprof, thr_data->papiex_mpisyncprof);
    }
    if (!no_io_prof)
      min_data->papiex_ioprof = min(min_data->papiex_ioprof, thr_data->papiex_ioprof);
    if (!no_threadsync_prof)
      min_data->papiex_threadsyncprof = min(min_data->papiex_threadsyncprof, thr_data->papiex_threadsyncprof);
    for (k=0;k<eventcnt;k++)
      min_data->data[0].counters[k] = min(min_data->data[0].counters[k], thr_data->data[0].counters[k]);
    for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
      if (thr_data->data[j].used >0) {
        min_data->data[j].used = min(min_data->data[j].used, thr_data->data[j].used);
        min_data->data[j].real_cyc = min(min_data->data[j].real_cyc, thr_data->data[j].real_cyc);
#ifdef FULL_CALIPER_DATA
        min_data->data[j].real_usec = min(min_data->data[j].real_usec, thr_data->data[j].real_usec);
        min_data->data[j].virt_usec = min(min_data->data[j].virt_usec, thr_data->data[j].virt_usec);
        min_data->data[j].virt_cyc = min(min_data->data[j].virt_cyc, thr_data->data[j].virt_cyc);
#endif
        for (k=0;k<eventcnt;k++)
          min_data->data[j].counters[k] = min(min_data->data[j].counters[k], thr_data->data[j].counters[k]);
      }
    } /* calipers */
  } /* nthreads */

  //printf("START max rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  /* max computing */
  LIBPAPIEX_DEBUG("\tComputing max. stats for rank %d\n", rank);
  thr_data = process->data[0];
  max_data->data[0].real_usec = thr_data->data[0].real_usec;
  max_data->data[0].real_cyc = thr_data->data[0].real_cyc;
  max_data->data[0].virt_usec = thr_data->data[0].virt_usec;
  max_data->data[0].virt_cyc = thr_data->data[0].virt_cyc;
  if (!no_mpi_prof && is_mpied) {
    max_data->papiex_mpiprof = thr_data->papiex_mpiprof;
    max_data->papiex_mpisyncprof = thr_data->papiex_mpisyncprof;
  }
  if (!no_io_prof)
    max_data->papiex_ioprof = thr_data->papiex_ioprof;
  if (!no_threadsync_prof)
    max_data->papiex_threadsyncprof = thr_data->papiex_threadsyncprof;
  for (k=0;k<eventcnt;k++)
    max_data->data[0].counters[k] = thr_data->data[0].counters[k];
  for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
    if (thr_data->data[j].used >0) {
      max_data->data[j].used = thr_data->data[j].used;
      max_data->data[j].real_cyc = thr_data->data[j].real_cyc;
#ifdef FULL_CALIPER_DATA
      max_data->data[j].real_usec = thr_data->data[j].real_usec;
      max_data->data[j].virt_usec = thr_data->data[j].virt_usec;
      max_data->data[j].virt_cyc = thr_data->data[j].virt_cyc;
#endif
      for (k=0;k<eventcnt;k++)
        max_data->data[j].counters[k] = thr_data->data[j].counters[k];
    }
  } /* calipers */

  //printf("START max rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  for (i=1; i<nthreads; i++) {
    thr_data = process->data[i];
    LIBPAPIEX_DEBUG("\t\tProcessing thread %d for rank %d\n", i, rank);
    max_data->data[0].real_usec = max(max_data->data[0].real_usec, thr_data->data[0].real_usec);
    max_data->data[0].real_cyc = max(max_data->data[0].real_cyc, thr_data->data[0].real_cyc);
    max_data->data[0].virt_usec = max(max_data->data[0].virt_usec, thr_data->data[0].virt_usec);
    max_data->data[0].virt_cyc = max(max_data->data[0].virt_cyc, thr_data->data[0].virt_cyc);
    if (!no_mpi_prof && is_mpied) {
      max_data->papiex_mpiprof = max(max_data->papiex_mpiprof, thr_data->papiex_mpiprof);
      max_data->papiex_mpisyncprof = max(max_data->papiex_mpisyncprof, thr_data->papiex_mpisyncprof);
    }
    if (!no_io_prof)
      max_data->papiex_ioprof = max(max_data->papiex_ioprof, thr_data->papiex_ioprof);
    if (!no_threadsync_prof)
      max_data->papiex_threadsyncprof = max(max_data->papiex_threadsyncprof, thr_data->papiex_threadsyncprof);
    for (k=0;k<eventcnt;k++)
      max_data->data[0].counters[k] = max(max_data->data[0].counters[k], thr_data->data[0].counters[k]);
    for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
      if (thr_data->data[j].used >0) {
        max_data->data[j].used = max(max_data->data[j].used, thr_data->data[j].used);
        max_data->data[j].real_cyc = max(max_data->data[j].real_cyc, thr_data->data[j].real_cyc);
#ifdef FULL_CALIPER_DATA
        max_data->data[j].real_usec = max(max_data->data[j].real_usec, thr_data->data[j].real_usec);
        max_data->data[j].virt_usec = max(max_data->data[j].virt_usec, thr_data->data[j].virt_usec);
        max_data->data[j].virt_cyc = max(max_data->data[j].virt_cyc, thr_data->data[j].virt_cyc);
#endif
        for (k=0;k<eventcnt;k++)
          max_data->data[j].counters[k] = max(max_data->data[j].counters[k], thr_data->data[j].counters[k]);
      }
    } /* calipers */
  } /* nthreads */

  /* stddev computation and stddev structure population */
  LIBPAPIEX_DEBUG("\tNow computing standard deviation for rank %d\n", rank);
  //printf("START squared_sums rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);

  for (i=0; i<nthreads; i++) {
    thr_data = process->data[i];
    LIBPAPIEX_DEBUG("\t\tProcessing thread %d for rank %d\n", i, rank);

    /* aggregate into the  global sums data structure */
    squared_sums.data[0].real_cyc += sqdiff_ll(mean_data->data[0].real_cyc,thr_data->data[0].real_cyc);
    squared_sums.data[0].real_usec += sqdiff_ll(mean_data->data[0].real_usec,thr_data->data[0].real_usec);
    squared_sums.data[0].virt_usec += sqdiff_ll(mean_data->data[0].virt_usec,thr_data->data[0].virt_usec);
    squared_sums.data[0].virt_cyc += sqdiff_ll(mean_data->data[0].virt_cyc,thr_data->data[0].virt_cyc);
    // printf("rank %d Here\n",get_rank());
    if (!no_mpi_prof && is_mpied) {
      squared_sums.papiex_mpiprof += sqdiff_ll(mean_data->papiex_mpiprof,thr_data->papiex_mpiprof);
      squared_sums.papiex_mpisyncprof += sqdiff_ll(mean_data->papiex_mpisyncprof,thr_data->papiex_mpisyncprof);
    }
    // printf("rank %d There\n",get_rank());
    if (!no_io_prof)
      squared_sums.papiex_ioprof += sqdiff_ll(mean_data->papiex_ioprof,thr_data->papiex_ioprof);
    if (!no_threadsync_prof)
      squared_sums.papiex_threadsyncprof += sqdiff_ll(mean_data->papiex_threadsyncprof,thr_data->papiex_threadsyncprof);
    // printf("rank %d Gone\n",get_rank());
    for (k=0;k<eventcnt;k++)
      squared_sums.data[0].counters[k] += sqdiff_ll(mean_data->data[0].counters[k],thr_data->data[0].counters[k]);
    for (j=1;j<PAPIEX_MAX_CALIPERS;j++) {
      if (thr_data->data[j].used >0) {
	// printf("rank %d Caliper\n",get_rank());
        squared_sums.data[j].used += sqdiff_ll(mean_data->data[j].used,thr_data->data[j].used);
        squared_sums.data[j].real_cyc += sqdiff_ll(mean_data->data[j].real_cyc,thr_data->data[j].real_cyc);
#ifdef FULL_CALIPER_DATA
        squared_sums.data[j].real_usec += sqdiff_ll(mean_data->data[j].real_usec,thr_data->data[j].real_usec);
        squared_sums.data[j].virt_usec += sqdiff_ll(mean_data->data[j].virt_usec,thr_data->data[j].virt_usec);
        squared_sums.data[j].virt_cyc += sqdiff_ll(mean_data->data[j].virt_cyc,thr_data->data[j].virt_cyc);
#endif
	for (k=0;k<eventcnt;k++)
          squared_sums.data[j].counters[k] += sqdiff_ll(mean_data->data[j].counters[k],thr_data->data[j].counters[k]);
      }
    } /* calipers */
  }

  /* divide by aggregates by nthreads to get std dev. */
  //printf("START stddev rank %d, threads %d\n",get_rank(),nthreads); fflush(stdout);
  stddev_data->data[0].real_usec = (long long) sqrt(squared_sums.data[0].real_usec / (double)nthreads) ; 
  stddev_data->data[0].real_cyc = (long long) sqrt(squared_sums.data[0].real_cyc / (double)nthreads) ; 
  stddev_data->data[0].virt_usec = (long long) sqrt(squared_sums.data[0].virt_usec / (double)nthreads) ; 
  stddev_data->data[0].virt_cyc = (long long) sqrt(squared_sums.data[0].virt_cyc / (double)nthreads) ; 
  // Why are these two not divided? 
//#warning "It makes no sense to me that these values are not divided by threads"
//#warning "it is different in global sums"
  if (!no_mpi_prof && is_mpied && (squared_sums.papiex_mpiprof!=0)) 
    stddev_data->papiex_mpiprof = (long long) sqrt(squared_sums.papiex_mpiprof / (double)nthreads) ; 
  if (!no_mpi_prof && is_mpied && (squared_sums.papiex_mpisyncprof!=0)) 
    stddev_data->papiex_mpisyncprof = (long long) sqrt(squared_sums.papiex_mpisyncprof / (double)nthreads) ; 
  if (!no_io_prof && (squared_sums.papiex_ioprof!=0)) 
    stddev_data->papiex_ioprof = (long long) sqrt(squared_sums.papiex_ioprof / (double)nthreads) ; 
  if (!no_threadsync_prof && (squared_sums.papiex_threadsyncprof!=0)) 
    stddev_data->papiex_threadsyncprof = (long long) sqrt(squared_sums.papiex_threadsyncprof / (double)nthreads) ; 
  for (k=0;k<eventcnt;k++)
    stddev_data->data[0].counters[k] = (long long) sqrt(squared_sums.data[0].counters[k] / (double)nthreads);
  for (j=1;j<PAPIEX_MAX_CALIPERS; j++) {
    if (squared_sums.data[j].used >0) {
      //printf("USED rank %d, threads %d, caliper %d, used %d, real %lld\n",get_rank(),nthreads,j,squared_sums.data[j].used,squared_sums.data[j].real_cyc); fflush(stdout);
//#warning "Why is this not divided by nthreads?"
      stddev_data->data[j].used = sqrt(squared_sums.data[j].used/(double)nthreads);
      stddev_data->data[j].real_cyc = (long long) sqrt(squared_sums.data[j].real_cyc / (double)nthreads) ; 
#ifdef FULL_CALIPER_DATA
      stddev_data->data[j].real_usec = (long long) sqrt(squared_sums.data[j].real_usec / (double)nthreads) ; 
      stddev_data->data[j].virt_usec = (long long) sqrt(squared_sums.data[j].virt_usec / (double)nthreads) ; 
      stddev_data->data[j].virt_cyc = (long long) sqrt(squared_sums.data[j].virt_cyc / (double)nthreads) ; 
#endif
      for (k=0;k<eventcnt;k++) {
	//printf("USED rank %d, threads %d, caliper %d, counter %lld\n",get_rank(),nthreads,j,squared_sums.data[j].counters[k]); fflush(stdout);
        stddev_data->data[j].counters[k] = (long long) sqrt(squared_sums.data[j].counters[k] / (double)nthreads);
      }
    }
  }
  //printf("FINISHED stddev rank %d\n",get_rank()); fflush(stdout);

  /* Finished calculating mean and stddev */

  LIBPAPIEX_DEBUG("\tNow printing statistics for rank %d\n", rank);
  // We don't print to stderr for MPI programs, only to a file
  if (output != NULL && !((output==stderr) && is_mpied)) {
    if (!quiet) {
      print_executable_info(output);
      fprintf(output,"%-30s: %d\n", "Num. of threads", nthreads);
      fprintf(output,"%-30s: %s", "Start",ctime(&proc_init_time.tv_sec));
      fprintf(output,"%-30s: %s", "Finish",ctime(&proc_fini_time.tv_sec));
    }
    fprintf(output, "\n");
    print_all_pretty_stats(output, process_data_sums, PROC_SCOPE, 0);  // last arg is ignored
    print_min_max_mean_cv(output, min_data, max_data, mean_data, stddev_data);
    fprintf(output, "Cumulative Process Counts:\n");
    print_counters(output, process_data_sums);
  } /* output != NULL */

  /* also write the above output to file */
  if (!no_write) {
    char proc_file[PATH_MAX];
    sprintf(proc_file, "%s/process_summary.txt", proc_stats_path);
    unlink(proc_file);
    FILE *proc_out = fopen(proc_file, "w");
    if (!proc_out) {
      LIBPAPIEX_ERROR("fopen(%s) failed. %s",
              proc_file, strerror(errno));
      monitor_real_exit(1);
    }
    if (!quiet) {
      print_executable_info(proc_out);
      fprintf(proc_out,"%-30s: %d\n", "Num. of threads", nthreads);
      fprintf(proc_out,"%-30s: %s", "Start",ctime(&proc_init_time.tv_sec));
      fprintf(proc_out,"%-30s: %s", "Finish",ctime(&proc_fini_time.tv_sec));
    }
    fprintf(proc_out, "\n");
    print_all_pretty_stats(proc_out, process_data_sums, PROC_SCOPE, 0); // last arg is ignored
    print_min_max_mean_cv(proc_out, min_data, max_data, mean_data, stddev_data);
    fprintf(proc_out, "Cumulative Process Counts:\n");
    print_counters(proc_out, process_data_sums);
    fclose(proc_out);
  }

  return;
}


/* This function should not probably be used to print
 * rusage and memory info, as that may not be thread-specific
 */
static void print_thread_stats(FILE *output, papiex_perthread_data_t *thread,
                               unsigned long tid, int scope, float process_walltime_usec)
{
  if (output == NULL) return;

  if (!quiet) { 
    print_executable_info(output);
    if (is_mpied) fprintf(output,"%-30s: %d\n", "Mpi Rank", myrank);
    if (_papiex_threaded) fprintf(output,"%-30s: %lu\n", "Thread id", tid);
#ifdef HAVE_PAPI
    fprintf(output, "%-30s: %s", "Start",ctime(&thread->stamp));
    fprintf(output, "%-30s: %s", "Finish",ctime(&thread->finish));
#endif
    fprintf(output, "\n");
  }

  if (!papiex_nextgen)
    print_all_pretty_stats(output, thread, scope, process_walltime_usec);

  /* check if we should be printing rusage and memory stuff here */
  if (rusage) {
    print_rusage_stats(output);
  }
  if (memory) {
    _papiex_dump_memory_info(output);
  }

  print_counters(output, thread);

  return;
}

void init_process_globals(void) {
  called_process_shutdown = 0; /* ensure shutdown is not attempted twice */
  monitor_fini_mpi_callback_occurred = 0; /* did the callback occur? */
  process_name = NULL;
  process_args[0] = '\0';
}

void init_library_globals(void) {
  /* Init all globals */
  memset(&all_threads,0x0,sizeof(all_threads));
  all_threads_size = PAPIEX_INIT_THREADS;
  clockrate = 0.0;
  csv_output = 0;
  dlproc_handle = NULL;
  domain = 0;
  eventcnt = 0;
  exeinfo = NULL;
  memset(eventnames,0x0,sizeof(char *)*PAPIEX_MAX_COUNTERS);
  memset(eventcodes,0x0,sizeof(int)*PAPIEX_MAX_COUNTERS);
  file_output = NULL;
  file_prefix = 0;
  fullname = NULL;
  gcc_prof = 0;
  hostname[0] = '\0';
  hwinfo = NULL;
  init_virt_cyc = 0;
  is_mpied = 0; 
  quiet = 0;
  memory = 0;
  mpx_interval= PAPIEX_DEFAULT_MPX_HZ; // in Hertz
  multiplex = 0;
  no_derived_stats = 0;
  no_follow_fork = 0;
  no_mpi_gather = 0; /* gather stats using MPI (only for classic) */
  no_scientific = 0;
  no_summary_stats = 0; /* don't generate summary statistics */
  no_write = 0;
  nthreads = 1;
  rusage = 0;
  pathscale_prof = PATHSCALE_NONE;
  papi_broken_stop =0 ;
  papi_mpx_zero_counts_bug = 0;
  _papiex_threaded = 0;
  papiex_nextgen = 0;
  processor = NULL;
  _papiex_debug = 0;
  thread_data = NULL;
  thread_id_handle = NULL;
  write_only = 0; /* no output to console */

  /* To satisfy eventinfo.c used in libpapiex.so */
  tool = "papiex";
  build_date = __DATE__;
  build_time = __TIME__;

  /* output */
  base_output_path[0] = '\0';
  proc_output_path[0] = '\0';
  thread_output_path[0] = '\0';
  output_file_name = NULL;
  file_prefix = 0;
  proc_stats_path[0] = '\0';
  user_path_prefix[0] = '\0';

#ifdef PROFILING_SUPPORT
  no_mpi_prof = 0; 
  no_io_prof = 0; 
  no_threadsync_prof = 1; 
#else
  no_mpi_prof = 1; 
  no_io_prof = 1; 
  no_threadsync_prof = 1;
#endif

  init_process_globals();
}

void monitor_init_library(void) {
  int retval;
  char *opts = NULL, *tmp = NULL;

  init_library_globals();

  opts = getenv(PAPIEX_ENV);
  if (opts == NULL)
    {
      LIBPAPIEX_ERROR("Environment variable %s is not defined.",PAPIEX_ENV);
      monitor_real_exit(1);
    }
  opts = strdup(opts);

  tmp = strtok(opts,", ");
  if (tmp) 
    {
      do {
	if (strcmp(tmp,"QUIET") == 0) {
	  //quiet = 1;
        }
	else if (strcmp(tmp,"DEBUG") == 0)
	  _papiex_debug++;
	else if (strcmp(tmp,"NO_MPI_GATHER") == 0)
	  no_mpi_gather = 1;
	else if (strcmp(tmp,"NO_DERIVED_STATS") == 0) 
	  no_derived_stats = 1;
	else if (strcmp(tmp,"NO_SUMMARY_STATS") == 0)
	  no_summary_stats = 1;
	else if (strcmp(tmp,"MULTIPLEX") == 0)
	  multiplex = 1;
        else if (strncmp(tmp,"MPX_INTERVAL=", 13)==0)
          mpx_interval = atoi(&tmp[13]);
        else if (strncmp(tmp,"SPEC=", 5)==0) {
          spec_file = strdup(&tmp[5]);
          user_specified_spec=1;
        }
	else if (strcmp(tmp,"NOFORK") == 0)
	  no_follow_fork = 1;
	else if (strcmp(tmp,"RUSAGE") == 0)
	  rusage = 1;
	else if (strcmp(tmp,"MEMORY") == 0)
	  memory = 1;
	else if (strcmp(tmp,"NO_WRITE") == 0)
	  no_write = 1;
	else if (strcmp(tmp,"GCC_PROF") == 0)
	  gcc_prof = 1;
	else if (strcmp(tmp,"PATHSCALE_FUNCS") == 0)
	  pathscale_prof = PATHSCALE_FUNCS;
	else if (strcmp(tmp,"PATHSCALE_ALL") == 0)
	  pathscale_prof = PATHSCALE_ALL;
	else if (strcmp(tmp,"PATHSCALE_LOOPS") == 0)
	  pathscale_prof = PATHSCALE_LOOPS;

        /* If these two are set, then papiex will
         * not compute %MPI cycles and %I/O cycles.
         * This might be needed if you don't want to
         * override the I/O / MPI calls
         */
	else if (strcmp(tmp,"NO_MPI_PROF") == 0)
          no_mpi_prof = 1;
	else if (strcmp(tmp,"NO_IO_PROF") == 0)
	  no_io_prof = 1;
	else if (strcmp(tmp,"NO_THREADSYNC_PROF") == 0)
	  no_threadsync_prof = 1;
	else if (strcmp(tmp,"NO_SCIENTIFIC") == 0) {
          no_scientific = 1;
        }
	else if (strcmp(tmp,"NEXTGEN") == 0) {
          papiex_nextgen = 1;
        }
	else if (strcmp(tmp,"CSV") == 0) {
          csv_output = 1;
          papiex_nextgen = 1; /* --csv does not work with --classic */
        }
	else if (strcmp(tmp,"WRITE_ONLY") == 0)
	  write_only = 1;
	else if (strcmp(tmp,"PREFIX") == 0)
	  {
	    char *t = getenv(PAPIEX_OUTPUT_ENV);

	    if ((t != NULL) && (strlen(t) > 0))
	      {
		file_output = strdup(t);
		file_prefix = 1;
                strcpy(user_path_prefix, t);
	      }
	  }
	else if (strcmp(tmp,"DIR") == 0)
	  {
	    char *t = getenv(PAPIEX_OUTPUT_ENV);

	    if ((t != NULL) && (strlen(t) > 0))
	      {
		sprintf(user_path_prefix, "%s/", t);
                if(mkdir(user_path_prefix, 0755))
                  if (errno != EEXIST) {
                    LIBPAPIEX_ERROR("mkdir(%s) failed. %s", user_path_prefix, strerror(errno));
                    monitor_real_exit(1);
                  }
		file_output = strdup(user_path_prefix);
		file_prefix = 1;
	      }
	  }
	else if (strcmp(tmp,"FILE") == 0)
	  {
	    char *t = getenv(PAPIEX_OUTPUT_ENV);

	    if ((t != NULL) && (strlen(t) > 0))
	      {
		file_output = strdup(t);
                output_file_name = file_output; 
		file_prefix = 0;
	      }
	  }
	else if (strcmp(tmp,"USER") == 0)
	  domain |= PAPI_DOM_USER;
	else if (strcmp(tmp,"KERNEL") == 0)
	  domain |= PAPI_DOM_KERNEL;
	else if (strcmp(tmp,"OTHER") == 0)
	  domain |= PAPI_DOM_OTHER;
	else if (strcmp(tmp,"SUPERVISOR") == 0)
	  domain |= PAPI_DOM_SUPERVISOR;
	else
	  {
            /* this is an event */
            if (strncmp(tmp,"PAPI_", 5) == 0) {
              // use PAPI-presets spec file unless the user
              // has explicitly chosen one
              if (!spec_file) spec_file = DEFAULT_SPEC_FILE;
            }
	    eventnames[eventcnt] = tmp;
	    eventcnt++;
	  }
      } while ((tmp = strtok(NULL,",")) != NULL);
    }
#ifndef HAVE_BINUTILS
  if ((gcc_prof != 0) || (pathscale_prof != PATHSCALE_NONE))
    LIBPAPIEX_ERROR("Gcc / Pathscale profiling require papiex to be compiled with binutils");
#endif
  if (papiex_nextgen) no_write = 0; // ignore NO_WRITE for nextgen papiex

  gethostname(hostname,256);

  if (_papiex_debug)
    {
      retval = PAPI_set_debug(PAPI_VERB_ECONT);
      if (retval != PAPI_OK)
	{
	  LIBPAPIEX_PAPI_ERROR("PAPI_set_debug",retval);
	  return;
	}
    }

  if (_papiex_debug > 4)
    putenv("PAPI_DEBUG=ALL");
  else if (_papiex_debug > 3)
    putenv("PAPI_DEBUG=API:THREADS:INTERNAL:SUBSTRATE");
  else if (_papiex_debug > 2)
    putenv("PAPI_DEBUG=API:SUBSTRATE");
  else if (_papiex_debug > 1)
    putenv("PAPI_DEBUG=API");
  

  if (dlproc_handle == NULL)
    dlproc_handle = monitor_real_dlopen(NULL, RTLD_LAZY);
  if (dlproc_handle == NULL) {
    LIBPAPIEX_ERROR("dlopen(NULL, RTLD_LAZY) failed. %s",  strerror(errno));
    monitor_real_exit(1);
  }

  if (!no_io_prof) {
    GET_DLSYM(dlproc_handle,"papiex_ioprof_init",fptr_papiex_ioprof_init,papiex_ioprof_init_fptr_t);
    if (!fptr_papiex_ioprof_init) no_io_prof = 1;
    else {
      GET_DLSYM(dlproc_handle,"papiex_ioprof_set_gate",fptr_papiex_ioprof_set_gate,papiex_ioprof_set_gate_fptr_t);
    }
  }

  if (!no_threadsync_prof) {
    GET_DLSYM(dlproc_handle,"papiex_threadsyncprof_init",fptr_papiex_threadsyncprof_init,papiex_threadsyncprof_init_fptr_t);
    if (!fptr_papiex_threadsyncprof_init) no_threadsync_prof = 1;
    else { 
      GET_DLSYM(dlproc_handle,"papiex_threadsyncprof_set_gate",fptr_papiex_threadsyncprof_set_gate,papiex_threadsyncprof_set_gate_fptr_t);
    }
  }
 
  if (!no_mpi_prof) { 
    GET_DLSYM(dlproc_handle,"papiex_mpiprof_init",fptr_papiex_mpiprof_init,papiex_mpiprof_init_fptr_t);
    if (!fptr_papiex_mpiprof_init) no_mpi_prof = 1;
    else {
      GET_DLSYM(dlproc_handle,"papiex_mpiprof_set_gate",fptr_papiex_mpiprof_set_gate,papiex_mpiprof_set_gate_fptr_t);
    }
  }
  LIBPAPIEX_DEBUG("no_io_prof: %d\n", no_io_prof);
  LIBPAPIEX_DEBUG("no_mpi_prof: %d\n", no_mpi_prof);

  LIBPAPIEX_DEBUG("Gcc profiling: %d\n",gcc_prof);
  if (gcc_prof) {
    GET_DLSYM(dlproc_handle,"papiex_gcc_set_gate",fptr_papiex_gcc_set_gate,papiex_gcc_set_gate_fptr_t);
    GET_DLSYM(dlproc_handle,"papiex_gcc_thread_init",fptr_papiex_gcc_thread_init,papiex_gcc_thread_init_fptr_t);
    GET_DLSYM(dlproc_handle,"papiex_gcc_thread_shutdown",fptr_papiex_gcc_thread_shutdown,papiex_gcc_thread_shutdown_fptr_t);
  }
 
  if (pathscale_prof) { 
    GET_DLSYM(dlproc_handle,"papiex_pathscale_set_gate",fptr_papiex_pathscale_set_gate,papiex_pathscale_set_gate_fptr_t);
    GET_DLSYM(dlproc_handle,"papiex_pathscale_thread_init",fptr_papiex_pathscale_thread_init,papiex_pathscale_thread_init_fptr_t);
    GET_DLSYM(dlproc_handle,"papiex_pathscale_thread_shutdown",fptr_papiex_pathscale_thread_shutdown,papiex_pathscale_thread_shutdown_fptr_t);
  }

  if ((no_io_prof == 0) && (fptr_papiex_ioprof_init))
    fptr_papiex_ioprof_init();

  if ((no_threadsync_prof == 0) && (fptr_papiex_threadsyncprof_init))
    fptr_papiex_threadsyncprof_init();

  if ((no_mpi_prof == 0) && (fptr_papiex_mpiprof_init))
    fptr_papiex_mpiprof_init();

  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE,&rl) < 0)
    LIBPAPIEX_WARN("getrlimit(RLIMIT_NOFILE): %s\n",strerror(errno));
  rl.rlim_cur = rl.rlim_max;
  if (setrlimit(RLIMIT_NOFILE,&rl) < 0)
    LIBPAPIEX_WARN("setrlimit(RLIMIT_NOFILE): returned %s\n",strerror(errno));
  LIBPAPIEX_DEBUG("RLIMIT_NOFILE cur %d, max %d\n",(int)rl.rlim_cur,(int)rl.rlim_max);
}
void* monitor_init_process(int *argc, char **argv, void* data)
{
  int i;
  LIBPAPIEX_DEBUG("monitor_init_process callback occurred\n");
  init_process_globals();
  for (i=1;i<*argc;i++)
    {
      if (i > 1)
	strcat(process_args," ");
      strcat(process_args,argv[i]);
    }
  papiex_process_init_routine();
  return (data);
}

void monitor_fini_process(int how, void* data)
{
  LIBPAPIEX_DEBUG("monitor_fini_process callback occurred\n");
  papiex_process_shutdown_routine();
}

void monitor_init_thread_support(void)
{
  int retval;
  LIBPAPIEX_DEBUG("monitor_init_thread_support callback occurred\n");
  retval = PAPI_thread_init((unsigned long (*)(void))monitor_get_thread_num);
  LIBPAPIEX_DEBUG("returned from PAPI_thread_init\n");
  if (retval != PAPI_OK)
    {
      LIBPAPIEX_PAPI_ERROR("PAPI_thread_init",retval);
      return;
    }
  _papiex_threaded = 1;
}

void *monitor_init_thread(int tid, void* data)
{
  LIBPAPIEX_DEBUG("monitor_init_thread callback occurred\n");
  papiex_thread_init_routine();
  return(NULL);
}

void monitor_fini_thread(void *data)
{
  LIBPAPIEX_DEBUG("monitor_fini_thread callback occurred\n");
  papiex_thread_shutdown_routine();
}

static void print_banner(FILE *stream) {
  assert(stream);
  assert(tool);
  assert(PAPIEX_VERSION);
  assert(build_date);
  assert(build_time);
  char email[1024] = "";
#ifdef MAILBUGS
  sprintf(email, "Send bug reports to %s", MAILBUGS);
#endif
  fprintf(stream, "%s:\n%s: %s (Build %s/%s)\n%s: %s\n",
                   tool, tool, PAPIEX_VERSION, build_date, 
                   build_time, tool, email);
  fflush(stream);
}
