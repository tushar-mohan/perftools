#include "papiex_internal.h"

#ifndef PAPIEX_SHARED_LIBS
#define PAPIEX_SHARED_LIBS "libpapiex.so libmonitor.so libpapi.so"
#endif

/* processor-specific useful native events lists */
static const char mips_25Kf_useful_events[] = "INSNS_FETCHED_FROM_ICACHE,INSN_ISSUED,STORE_INSNS_ISSUED,L2_MISSES,BRANCHES_JUMPS_ISSUED,INSNS_COMPLETE,CACHEABLE_DCACHE_REQUEST,DCACHE_WRITEBACKS,ICACHE_MISSES,LOAD_STORE_ISSUED,L2_WBACKS,JR_COMPLETED,JR_RPD_MISSPREDICTED,INT_INSNS_ISSUED,FP_INSNS_ISSUED,BRANCHES_MISSPREDICTED,CACHEABLE_L2_REQS,UTLB_MISSES,LOAD_INSNS_ISSUED,CYCLES,BRANCHES_COMPLETED,INSN_FP_DATAPATH_COMPLETED,DCACHE_MISSES";
static const char sc_ice9A_useful_events[] = "CPU_BRANCH,CPU_CYCLES,CPU_DCEVICT,CPU_DCMISS,CPU_DTLBMISS,CPU_FLOAT,CPU_ICMISS,CPU_INSDUAL,CPU_INSEXEC,CPU_ITLBMISS,CPU_LOAD,CPU_MISPRED,CPU_MSTALL,CPU_SC,CPU_SCFAIL,CPU_STORE,CPU_TLBTRAP";
static const char sc_ice9B_useful_events[] = "CPU_BRANCH,CPU_CYCLES,CPU_DCEVICT,CPU_DCMISS,CPU_DTLBMISS,CPU_FLOAT,CPU_ICMISS,CPU_INSDUAL,CPU_INSEXEC,CPU_ITLBMISS,CPU_LOAD,CPU_MISPRED,CPU_MSTALL,CPU_SC,CPU_SCFAIL,CPU_STORE,CPU_TLBTRAP,CPU_FPARITH,CPU_FPMADD,CPU_L2MISS,CPU_L2MISSALL,CPU_L2REQ";
static const char mips_74K_useful_events[] = "CYCLES,INSTRUCTIONS,PREDICTED_JR_31,JR_31_MISPREDICTIONS,REDIRECT_STALLS,JR_31_NO_PREDICTIONS,ITLB_ACCESSES,ITLB_MISSES,JTLB_INSN_MISSES,ICACHE_ACCESSES,ICACHE_MISSES,ICACHE_MISS_STALLS,DDQ0_FULL_DR_STALLS,DDQ1_FULL_DR_STALL,ALCB_FULL_DR_STALLS,AGCB_FULL_DR_STALLS,CLDQ_FULL_DR_STALLS,IODQ_FULL_DR_STALLS,ALU_NO_ISSUE_CYCLES,AGEN_NO_ISSUE_CYCLES,SINGLE_ISSUE_CYCLES,DUAL_ISSUE_CYCLES,DCACHE_LOAD_ACCESSES,DCACHE_ACCESSES,DCACHE_WRITEBACKS,DCACHE_MISSES,JTLB_DATA_ACCESSES,JTLB_DATA_MISSES,L2_CACHE_WRITEBACKS,L2_CACHE_ACCESSES,L2_CACHE_MISSES,FSB_FULL_STALLS,LDQ_FULL_STALLS,WBB_FULL_STALLS,JR_NON_31_INSNS,MISPREDICTED_JR_31_INSNS,COND_BRANCH_INSNS,MISPREDICTED_BRANCH_INSNS,INTEGER_INSNS,FPU_INSNS,LOAD_INSNS,STORE_INSNS,J_JAL_INSNS,MIPS16_INSNS,NT_MUL_DIV_INSNS,UNCACHED_LOAD_INSNS,UNCACHED_STORE_INSNS,SC_INSNS,PREFETCH_INSNS,CACHE_HIT_PREFETCH_INSNS,NO_INSN_CYCLES,LOAD_MISS_INSNS,ONE_INSN_CYCLES,TWO_INSNS_CYCLES,EXCEPTIONS_TAKEN,NO_INSTRUCTIONS_FROM_REPLAY_CYCLES,PERF_COUNT_SW_TASK_CLOCK,PERF_COUNT_SW_CONTEXT_SWITCHES,PERF_COUNT_SW_CPU_MIGRATIONS,PERF_COUNT_SW_PAGE_FAULTS_MIN,PERF_COUNT_SW_PAGE_FAULTS_MAJ,OOO_ALU_ISSUE_CYCLES,OOO_AGEN_ISSUE_CYCLES,GFIFO_BLOCKED_CYCLES";

// nehalem events
static const char corei7_events[] = "UNHALTED_CORE_CYCLES,INSTRUCTION_RETIRED,FP_COMP_OPS_EXE:SSE_DOUBLE_PRECISION,FP_COMP_OPS_EXE:SSE_SINGLE_PRECISION,BR_INST_RETIRED:ALL_BRANCHES,LLC_MISSES,L2_RQSTS:LOADS,L2_RQSTS:LD_HIT,L2_RQSTS:REFERENCES,L2_RQSTS:MISS,L2_DATA_RQSTS:ANY,L1I:HITS,L1I:MISSES,DTLB_LOAD_MISSES:WALK_COMPLETED,MEM_LOAD_RETIRED:LLC_MISS,MEM_LOAD_RETIRED:OTHER_CORE_L2_HIT_HITM,MEM_LOAD_RETIRED:DTLB_MISS,L1D_CACHE_LD:I_STATE,L1D_CACHE_LD:MESI,L1D_CACHE_ST:MESI,UOPS_ISSUED:STALLED_CYCLES,RESOURCE_STALLS:ANY,RESOURCE_STALLS:ROB_FULL,BR_MISP_EXEC:ANY,RESOURCE_STALLS:RS_FULL,UOPS_ISSUED:ANY";

// Westmere has slightly different events from the default nehalem (core i7)
static const char westmere_events[] = "UNHALTED_CORE_CYCLES,INSTRUCTION_RETIRED,FP_COMP_OPS_EXE:SSE_DOUBLE_PRECISION,FP_COMP_OPS_EXE:SSE_SINGLE_PRECISION,BR_INST_RETIRED:ALL_BRANCHES,LLC_MISSES,L2_RQSTS:LOADS,L2_RQSTS:LD_HIT,L2_RQSTS:REFERENCES,L2_RQSTS:MISS,L2_DATA_RQSTS:ANY,L1I:HITS,L1I:MISSES,DTLB_LOAD_MISSES:ANY,MEM_STORE_RETIRED:DTLB_MISS,MEM_LOAD_RETIRED:L3_MISS,MEM_LOAD_RETIRED:OTHER_CORE_L2_HIT_HITM,MEM_LOAD_RETIRED:DTLB_MISS,UOPS_ISSUED:STALL_CYCLES,RESOURCE_STALLS:ANY,RESOURCE_STALLS:ROB_FULL,BR_MISP_EXEC:ANY,RESOURCE_STALLS:RS_FULL,UOPS_EXECUTED:PORT015,UOPS_EXECUTED:PORT015_STALL_CYCLES,L1D:REPL,L1D_WB_L2:MESI,L2_LINES_IN:ANY";

/* This is an ordered list with most important to less important from left-to-right
 * In case there's a compatibility issue, events are added from left first */
static const char papi_useful_presets[] ="PAPI_TOT_INS,PAPI_FP_INS,PAPI_LST_INS,PAPI_BR_INS,PAPI_VEC_INS,PAPI_INT_INS,PAPI_FMA_INS,PAPI_LD_INS,PAPI_SR_INS,PAPI_TOT_CYC,PAPI_RES_STL,PAPI_MEM_SCY,PAPI_FP_STAL,PAPI_L1_DCM,PAPI_L1_ICM,PAPI_TLB_DM,PAPI_TLB_IM,PAPI_L2_DCM,PAPI_L2_ICM,PAPI_CA_INV,PAPI_CSR_FAL,PAPI_CSR_TOT,PAPI_LSU_IDL,PAPI_FPU_IDL,PAPI_STL_ICY,PAPI_FUL_ICY,PAPI_BR_CN,PAPI_BR_MSP,PAPI_FP_OPS,PAPI_L1_DCA,PAPI_L2_DCA,PAPI_L2_ICA";

static const char ENV_PAPIEX_LD_LIBPATH[] = "PAPIEX_LD_LIBRARY_PATH";

static int debug = 0;
static int shell_dump = 0;
static int no_ld_library_path = 0; /* should we modify LD_LIBRARY_PATH ? */
static int no_summary_stats = 0;
static int no_derived_stats = 0;
static int no_mpi_gather = 0; /* only used for classic papiex */
#ifdef PROFILING_SUPPORT
  static int no_mpi_prof = 0;
  static int no_io_prof = 0;
  static int no_threadsync_prof = 1;
#else
  static int no_mpi_prof = 1;
  static int no_io_prof = 1;
  static int no_threadsync_prof = 1;
#endif
static int use_papi_presets = 0;

static int test_events_only = 0;
static char incompatible_events[PATH_MAX];
static char unavailable_events[PATH_MAX];
static char successful_events[PATH_MAX];
static char requested_events[PATH_MAX];
static int could_not_add_some_events = 0;
static int skip_unavailable_events = 0;

static int gcc_prof = 0;
static int pathscale_prof = PATHSCALE_NONE;
static int no_scientific = 0; 
static int no_compat_check = 0; // test PAPI events for compatibility
static int classic = 0;         // use the old papiex mode or the nextgen?
static int csv_output = 0;
static int multiplex = 0;
static int auto_stats = 0;
static int user_set_event = 0; // did the user set at least one event?
static int new_ld_paths_added = 0;
const PAPI_hw_info_t *hwinfo = NULL;

/* Global used by error messages */
char *tool = "";

static const struct option gen_options[] = {
        {"help", 0, NULL, 'h'},
         {0, 0, 0, 0}
};


/* PAPIEX-specific */
static const struct option long_options[] = {
         {"no-ld-path", 0, &no_ld_library_path, 1},
         {"no-summary", 0, &no_summary_stats, 1},
         {"no-derived", 0, &no_derived_stats, 1},
         {"no-mpi-gather", 0, &no_mpi_gather, 1},
         {"use-papi-presets", 0, &use_papi_presets, 1},
         {"test", 0, &test_events_only, 1},
         {"skip-unavailable-events", 0, &skip_unavailable_events, 1},
#ifdef PROFILING_SUPPORT
         {"no-mpi-prof", 0, &no_mpi_prof, 1},
         {"no-io-prof", 0, &no_io_prof, 1},
         {"no-threadsync-prof", 0, &no_threadsync_prof, 1},
#endif
         {"no-scientific", 0, &no_scientific, 1},
         {"no-compat-check", 0, &no_compat_check, 1},
         {"classic", 0, &classic, 1},
         {"csv-output", 0, &csv_output, 1},
#ifdef HAVE_BINUTILS
         {"gnu", 0, NULL, 'G'},
         {"pathscale", 2, NULL, 'Q'},
#endif
         {"spec", 1, NULL, 'E'},
         {"help", 0, NULL, 'h'},
         {0, 0, 0, 0}
};


/* common functions needed for all tools */
static void dump_substrate_info (FILE *);
static void get_useful_events (char *events, int maxlen);
static int test_papi_events(const char* events);
static int make_compatible_eventset(char* events);
static inline void append_option(char *opt_str, const char* option) {
  if (strlen(opt_str))
    strcat(opt_str, ",");
  strcat(opt_str, option);
  return;
}

static int file_exists(const char * fileName) {
  struct stat buf;
  int i = stat (fileName, &buf);
  /* File found */
  if ( i == 0 ) return 1;
  return 0;
}

/* COMMON PAPI OPTION HANDLING */
void do_i_option(const PAPI_hw_info_t *hwi)
{
  printf("%-30s: %d.%d.%d.%d\n", "PAPI Version",
      PAPI_VERSION_MAJOR( PAPI_VERSION ),
      PAPI_VERSION_MINOR( PAPI_VERSION ),
      PAPI_VERSION_REVISION( PAPI_VERSION ),
      PAPI_VERSION_INCREMENT( PAPI_VERSION ) );
  printf ("%-30s: %s (%d)\n","Vendor string and code",
	  hwi->vendor_string, hwi->vendor);
  printf ("%-30s: %s (%d)\n","Model string and code",
	  hwi->model_string, hwi->model);
  if ( (hwi)->cpuid_family > 0 )
    printf ( "%-30s: Family: %d  Model: %d  Stepping: %d\n", "CPUID Info",
        hwi->cpuid_family, hwi->cpuid_model,
        (hwi)->cpuid_stepping );
  printf("%-30s: %f\n", "CPU Revision", (hwi)->revision );
  printf("%-30s: %d\n", "CPU Max Megahertz", (hwi)->cpu_max_mhz );
  printf("%-30s: %d\n", "CPU Min Megahertz", (hwi)->cpu_min_mhz );
  printf ("%-30s: %d\n","Total # of CPU's", hwi->totalcpus);
  if ( (hwi)->sockets > 0 )
    printf("%-30s: %d\n", "Sockets", (hwi)->sockets );
  if ( (hwi)->cores > 0 )
    printf("%-30s: %d\n", "Cores per Socket", (hwi)->cores );
  if ( (hwi)->threads > 0 )
    printf("%-30s: %d\n", "# Hdw Threads per core", (hwi)->threads );
  if ( (hwi)->nnodes > 0 )
    printf("%-30s: %d\n", "NUMA Nodes", (hwi)->nnodes );
  printf("%-30s: %d\n", "CPUs per Node", (hwi)->ncpu );
  printf("%-30s: %s\n", "Running in a VM", (hwi)->virtualized?  "yes":"no");
  if ((hwi)->virtualized) 
    printf("%-30s: %s\n", "VM Vendor", (hwi)->virtual_vendor_string);
  printf ("%-30s: %d\n","Number Hardware Counters", PAPI_get_opt (PAPI_MAX_HWCTRS, NULL));
  printf ("%-30s: %d\n","Papiex Max Multiplex Counters", PAPIEX_MAX_COUNTERS);

  dump_substrate_info (stdout);
}

/* If NULL, list-em-all */
void do_L_option(const PAPI_hw_info_t *hwi, const char *event)
{
  int eventcode, retval;
  
  if (event)
    {
      retval = PAPI_event_name_to_code ((char *)event, &eventcode);
      if (retval != PAPI_OK) {
        PAPIEX_WARN("Could not map event to code for %s\n", (char *)event);
	PAPIEX_PAPI_ERROR ("PAPI_event_name_to_code", retval);
      }
      
       _papiex_dump_event_info (stdout, eventcode, 1);
    }
  else
    {
      int i;
      PAPI_event_info_t info;
      printf ("Preset events:\n\n");
      printf ("Preset                         Description\n");

      i = 0 | PAPI_PRESET_MASK;
      PAPI_enum_event(&i, PAPI_ENUM_FIRST);
      do
	{
	  if (PAPI_get_event_info (i, &info) == PAPI_OK)
	    {
	      if (info.count)
		printf ("%-30s %s\n", info.symbol,
			info.long_descr);
	    }
	}
      while (PAPI_enum_event (&i, PAPI_PRESET_ENUM_AVAIL) == PAPI_OK);

      printf ("\nNative events:\n\n");
      printf ("Native                         Description\n");

      i = 0 | PAPI_NATIVE_MASK;
      PAPI_enum_event(&i, PAPI_ENUM_FIRST);
      do {
	      if (PAPI_get_event_info (i, &info) == PAPI_OK)
		{
		  printf ("%-30s %s\n", info.symbol,
			  info.long_descr);
		}
	    }
	  while (PAPI_enum_event (&i, PAPI_ENUM_EVENTS) == PAPI_OK);
    }
}

void do_V_option(const char *name, int version)
{
#ifdef MMPERFTOOLS_VERSION
  printf("MMPerftools version %s\n", MMPERFTOOLS_VERSION);
#endif
  printf("%s driver version %s\n", name, PAPIEX_VERSION);
  printf("PAPI library version %u.%u.%u\nPAPI header version %u.%u.%u\n",
      PAPI_VERSION_MAJOR (version), PAPI_VERSION_MINOR (version),
      PAPI_VERSION_REVISION (version), PAPI_VERSION_MAJOR (PAPI_VERSION),
      PAPI_VERSION_MINOR (PAPI_VERSION), PAPI_VERSION_REVISION (PAPI_VERSION));
  printf("Build %s %s\n", __DATE__, __TIME__);
}

void do_s_option(void)
{
  shell_dump = 1;
}

int do_G_option(char *name, char *option_string)
{
#ifndef HAVE_BINUTILS
  PAPIEX_WARN("GCC instrumentation requires papiex to be compiled with binutils support\n");
  return 1;
#endif
  if (pathscale_prof)
    {
      PAPIEX_WARN ("-Q and -G are not compatible.\n");
      return 1;
    }
  if (strlen (option_string))
    strcat (option_string, ",");
  strcat (option_string, "GCC_PROF");
  gcc_prof = 1;
  return 0;
}

int do_Q_option(char *name, char *opt, char *option_string)
{
  char *tmp;
#ifndef HAVE_BINUTILS
  PAPIEX_WARN("Pathscale instrumentation requires papiex to be compiled with binutils support\n");
  return 1;
#endif
  if (gcc_prof)
    {
      PAPIEX_WARN ("-Q and -G are not compatible.\n");
      return 1;
    }
  if (opt)
    {
      if (strncasecmp(opt,"func",strlen("func")) == 0)
	pathscale_prof = PATHSCALE_FUNCS;
      else if (strncasecmp(opt,"loop",strlen("loop")) == 0)
	pathscale_prof = PATHSCALE_LOOPS;
      else if (strcasecmp(opt,"all") == 0)
	pathscale_prof = PATHSCALE_ALL;
      else
	{
	  PAPIEX_WARN ("Unknown pathscale profile type %s.\n",opt);
	  return 1;
	}
    }
  else
    pathscale_prof = PATHSCALE_ALL;
  
  switch (pathscale_prof)
    {
    case PATHSCALE_FUNCS:
      tmp = "PATHSCALE_FUNCS";
      break;
    case PATHSCALE_LOOPS:
      tmp = "PATHSCALE_LOOPS";
      break;
    case PATHSCALE_ALL:
    default:
      tmp = "PATHSCALE_ALL";
      break;
    }
  if (strlen (option_string))
    strcat (option_string, ",");
  strcat (option_string, tmp);
  return 0;
}

static int handle_common_opts(char c, const char* optarg, char **my_argv, int version) {
  int found = 1;
  switch(c) {
    case ':':
    case '?':
      {
        PAPIEX_ERROR ("Try '%s -h' for more information.\n",tool);
        exit (1);
        break;
      }

    case 'i':
      {
        do_i_option(hwinfo);
        exit (0);
        break;
      }

    case 'V':
      {
        do_V_option(my_argv[0],version);
        exit (0);
        break;
      }

    case 's':
      {
        do_s_option();
        break;
      }

    case 'L':
      {
        do_L_option(hwinfo,optarg);
        exit (0);
        break;
      }

    case 'l':
      {
        do_L_option(hwinfo,NULL);
        exit (0);
        break;
      }
    default:
      found = 0;
  }
  return found;
}


void print_common_opts(void)
{
  printf (" -d\t\tEnable debugging output, use repeatedly for more output.\n");
  printf (" -h\t\tPrint this message.\n");
  printf (" -i\t\tPrint information about the host machine.\n");
  printf (" -s\t\tDump the shell environment variables and exit.\n");
  printf (" -V\t\tPrint version information.\n");
}

void print_common_output_opts(void)
{
  printf (" -q\t\tQuiet mode; Suitable for parsing with other tools.\n");
  printf (" -n\t\tDo not write to files, write to stderr. Opposite of -w\n");
  printf ("\t\tThis is ignored unless --classic is used.\n");
  printf (" -w\t\tDo not write to stderr, write to files. Opposite of -n\n");
  printf ("\t\tThis is ignored unless --classic is used.\n");
  printf (" -f <dir>\tSend output to <dir>/<cmd>.%s.<host>.<pid>.<generation>\n", tool);
  printf (" -p <prefix>\tSend output to <prefix><cmd>.%s.<host>.<pid>.<generation>.\n", tool);
  printf (" -o <file>\tSend output to <file>.  If <file> exists, a suffix will\n"
  	  "\t\tbe appended, and the old file will not be overwritten.\n"
          "\t\tIt is recommended that -o is NOT used for runs where multiple\n"
	  "\t\tfiles are created (use -f instead in such situations).\n");
  printf ("\n\t\t*** The options -n, -p, -f and -o are mutually exclusive. ***\n\n");
}

void print_common_papi_opts(void)
{
  printf (" -e event\tMonitor this event. May be repeated.\n");
  printf (" -l\t\tPrint a terse listing of all the available events to count.\n");
  printf (" -L event\tPrint a full description of this event.\n");
  printf (" -m<interval>\tEnable counter multiplexing. Silently ignored if unavailable.\n"
          "\t\tThe 'interval' is in Hz, and is optional (defaults to 10).\n");
  printf (" -U\t\tMonitor user mode events (default).\n");
  printf (" -K\t\tMonitor kernel mode events\n");
  printf (" -I\t\tMonitor transient (interrupt) mode events.\n");
  printf (" -S\t\tMonitor supervisor mode events.\n");
  printf (" --test\tTest events for availability and compatibility and exit.\n");
  printf (" --skip-unavailable-events Skip events that are unavailable.\n");
}

void print_common_instr_opts(void)
{
#ifdef HAVE_BINUTILS
  printf (" -G --gnu\tEnable GNU compiler instrumentation.\n");
  printf (" -Q --pathscale[=loop,func,all]\tEnable Pathscale compiler instrumentation.\n");
#endif
}

void print_copyright(void)
{
  printf ("\n");
  printf ("This is OPEN SOURCE SOFTWARE written by Philip J. Mucci and Tushar Mohan.\n");
  printf ("This software is covered by the MIT License (https://opensource.org/licenses/MIT)\n");
#ifdef MAILBUGS
  printf ("\nPlease send all bug reports to: %s\n", MAILBUGS);
#else
  printf ("\nPlease send all bug reports to: ospat-devel@cs.utk.edu\n");
#endif
}

/* DO NOT SET ENVIRONMENT VARIABLES IN THIS ROUTINE, THAT HAPPENS IN MAIN */
static char * parse_args (int my_argc, char **my_argv, char **cmd, char ***cmd_argv,
	                        char *preload_env, char *preload_sep, char *ldpath_env, 
                          char *ldpath_sep, char *outstr)
{
  int c; 
  char events[PATH_MAX] = "";
  static char option_string[PATH_MAX] = "";
  int version = 0;
  PAPI_option_t pl;

  tool = my_argv[0];

  PAPI_set_debug(PAPI_VERB_ECONT);
  version = PAPI_library_init (PAPI_VER_CURRENT);
  if (version != PAPI_VER_CURRENT)
    PAPIEX_PAPI_ERROR ("PAPI_library_init", version);
  PAPI_set_debug(PAPI_QUIET);

  PAPI_get_opt (PAPI_PRELOAD, &pl);
  strcpy (preload_env, pl.preload.lib_preload_env);
  *preload_sep = pl.preload.lib_preload_sep;
  strcpy (ldpath_env, pl.preload.lib_dir_env);
  *ldpath_sep = pl.preload.lib_dir_sep;

  if ((hwinfo = PAPI_get_hardware_info ()) == NULL)
    PAPIEX_PAPI_ERROR ("PAPI_query_all_events_verbose", 0);

  if (my_argc)
    {
      while (1)
	{
	  c = getopt_long (my_argc, my_argv, "+dhisVqnwf:p:o:e:lL:m::UKISGQ::E:M::arx", // M::, 
                             long_options, NULL);

	  if (c == -1)
	    break;

	  if (handle_common_opts(c, optarg, my_argv, version))
	    continue; // if we found a matching option continue

	      switch (c)
		{
		case 'd':
		  append_option (option_string, "DEBUG");
		  PAPI_set_debug(PAPI_VERB_ECONT);
		  debug++;
		  break;
		case 'E':
                  if (optarg != NULL) {
                    if (!file_exists(optarg)) {
                      fprintf(stderr, "Spec-file %s, does not exist or is not readable\n", optarg);
                      goto error;
                    }
                    char spec_file[PATH_MAX];
                    sprintf(spec_file, "SPEC=%s", optarg);
                    append_option(option_string, spec_file);
                  }
                  else goto error;
		  break;
		case 'Q':
		  if (do_Q_option(my_argv[0],optarg,option_string))
		    goto error;
		  break;
		case 'G':
		  if (do_G_option(my_argv[0],option_string))
		    goto error;
		  break;
		case 'h':
		  printf
		    ("Usage:\n"
		     "papiex [-dhisVqnwlUKISEGQarx] [--classic] [-f output-dir] [-p prefix] [-o output-file]\n"
         "       [-e papi_event] [--test] [-m<interval>] [-L papi_event].. \n"
#ifdef HAVE_BINUTILS
         "       [--gnu] [--pathscale[=loop,func,all]\n"
#endif /* HAVE_BINUTILS */
#ifdef PROFILING_SUPPORT
         "       [--no-io-prof] [--no-threadsync-prof] [--no-mpi-prof]\n"
#endif /* PROFILING_SUPPORT */
		     "       [--no-mpi-gather] [--no-summary] [--no-derived] [--no-scientific] [--no-ld-path]\n"
		     "       [--no-compat-check] [--use-papi-presets] [--spec=spec-file] [--csv]\n"
         "       -- <cmd> <cmd options>\n\n");
		  print_common_opts();
		  print_common_output_opts();
		  print_common_papi_opts();
		  print_common_instr_opts();
		  printf (" --csv\t\tPrint machine-readable CSV output.\n"
              "\t\tThis option will silently ignore --classic.\n");
		  printf (" -a\t\tMonitor 'useful' events automatically (-m and -x are implicit).\n");
		  printf (" -r\t\tReport getrusage() information.\n");
		  printf (" -x\t\tReport memory usage information.\n");
		  printf (" --classic\tForce papiex to run in the old classic mode.\n");
#ifdef PROFILING_SUPPORT
		  printf (" --no-io-prof\tDisable I/O call profiling.\n");
		  printf (" --no-threadsync-prof Disable thread barrier & mutex call profiling.\n");
		  printf (" --no-mpi-prof\tDisable MPI call profiling.\n");
#endif /* PROFILING_SUPPORT */
		  printf (" --no-mpi-gather\tDo not gather statistics across tasks using MPI.\n"
              "\t\tThis option is only meaningful with --classic.\n");
		  printf (" --no-summary\tDo not generate any summary statistics (threads/MPI).\n");
		  printf (" --no-derived\tDo not generate any derived statistics.\n");
		  printf (" --no-scientific Do not print output numbers in scientific notation.\n");
		  printf (" --no-ld-path\tDo not modify the LD_LIBRARY_PATH variable at all.\n");
		  printf (" --no-compat-check Do not test events for compatibility\n"
              "\t\t*** You should use this option with caution, as PAPI calls\n"
              "\t\tmay not behave correctly with this option.\n"
              "\t\tIt's provided for debugging purposes.\n");
      printf (" --use-papi-presets Use PAPI preset events instead of\n"
              "\t\tprocessor-specific native events. Ignored unless -a used.\n");
      printf (" -E,--spec=<spec-file> Use the user-supplied processor spec file.\n");
		  printf("\nThe runtime loader MUST find %s.\n"
             "Set LD_LIBRARY_PATH appropriately if necessary.\n", PAPIEX_SHARED_LIBS);
		  print_copyright();
		  exit (0);
		  break;
    case 0: 
      /* do nothing; long option */
      break;
		case 'q':
		  if (strstr (option_string, "RUSAGE"))
		    {
		      PAPIEX_ERROR ("-r and -q are not compatible.\n");
		      exit (1);
		    }
		  append_option (option_string, "QUIET");
		  break;
		case 'm':
		  append_option (option_string, "MULTIPLEX");
		  multiplex = 1;
                  if (optarg != NULL) {
                    int val=strtol(optarg, (char **)NULL, 10);
                    if ((errno == ERANGE) || (val <= 0)) {
                      PAPIEX_ERROR("The argument to '-m' must be a positive integer\n");
                      exit(1);
                    }
                    char interval[25];
                    sprintf(interval, "MPX_INTERVAL=%d", val);
                    append_option(option_string, interval);
                  }
		  break;
		case 'r':
		  if (strstr (option_string, "QUIET"))
		    {
		      PAPIEX_ERROR ("-r and -q are not compatible.\n");
		      exit (1);
		    }
		  append_option (option_string, "RUSAGE");
		  break;
		case 'x':
		  if (strstr (option_string, "QUIET"))
		    {
		      PAPIEX_ERROR ("-x and -q are not compatible.\n");
		      exit (1);
		    }
		  append_option (option_string, "MEMORY");
		  break;
		case 'n':
		    if (strstr (option_string, "DIR") || strstr(option_string, "FILE") || strstr(option_string, "PREFIX") ||
                        strstr (option_string, "WRITE_ONLY"))
		      {
			PAPIEX_ERROR ("-n is not compatible with -f, -o, -p or -w.\n");
			exit (1);
		      }
		  append_option (option_string, "NO_WRITE");
		  break;
		case 'w':
		  if (strstr (option_string, "NO_WRITE"))
		    {
		      PAPIEX_ERROR ("-w and -n are not compatible.\n");
		      exit (1);
		    }
		  append_option (option_string, "WRITE_ONLY");
		  break;
		case 'U':
		  append_option (option_string, "USER");
		  break;
		case 'K':
		  append_option (option_string, "KERNEL");
		  break;
		case 'I':
		  append_option (option_string, "OTHER");
		  break;
		case 'S':
		  append_option (option_string, "SUPERVISOR");
		  break;
		case 'p':
		  {
		    if (strstr (option_string, "DIR") || strstr(option_string, "FILE") || strstr(option_string, "NO_WRITE"))
		      {
			PAPIEX_ERROR ("-p is not compatible with -f, -o and -n.\n");
			exit (1);
		      }
		    if (optarg == NULL)
		      {
			append_option (option_string, "PREFIX");
		      }
		    else
		      {
			append_option (option_string, "PREFIX");
			strncpy (outstr, optarg, PATH_MAX);
		      }
		  }
		  break;
		case 'f':
		  {
		    if (strstr (option_string, "PREFIX") || strstr(option_string, "FILE") || strstr(option_string, "NO_WRITE"))
		      {
			PAPIEX_ERROR ("-f is not compatible with -n, -p and -o.\n");
			exit (1);
		      }
		    append_option (option_string, "DIR");
		    strncpy (outstr, optarg, PATH_MAX);
		  }
		  break;
		case 'o':
		  {
		    if (strstr (option_string, "PREFIX") || strstr(option_string, "DIR") || strstr(option_string, "NO_WRITE"))
		      {
			PAPIEX_ERROR ("-o is not compatible with -n, -f and -p.\n");
			exit (1);
		      }
		    append_option (option_string, "FILE");
		    strncpy (outstr, optarg, PATH_MAX);
		  }
		  break;
		case 'a':
		  {
                    append_option (option_string, "MULTIPLEX");	/* multiplexing is implicit with -a */
                    append_option (option_string, "MEMORY");	/* memory is implicit with -a */
                    multiplex = 1;
                    auto_stats = 1;
                    skip_unavailable_events = 1;
		    break;
		  }
		case 'e':
		  {
		    append_option(events,optarg);
                    user_set_event=1;
		    break;
		  }
		default:
		  error:
		    PAPIEX_ERROR ("Try `%s -h' for more information.\n", my_argv[0]);
		    exit (1);
		  }
	    } /* while */
	}

  if (test_events_only)
    skip_unavailable_events = 1;

  /* auto stats */
  /* this has to be done after all the args are parsed, since
   * --use-papi-presets should modify the behavior of get_useful_events */
  if (auto_stats) {
    char useful_events[PATH_MAX];
    useful_events[0] = '\0';
    get_useful_events (useful_events, PATH_MAX);
    if (strlen (useful_events)) 
       append_option(events, useful_events);
  }
  else {
    // add architecture-specific events if the
    // user hasn't added any. This isn't necessary
    // if we don't choose any and the user doesn't
    // set any, then papiex.c will choose a couple
    // of PAPI presets
    //if (!user_set_event && strcmp(hwinfo->model_string, INTEL_COREI7_STRING)==0) {
    //   append_option(events, "UNHALTED_CORE_CYCLES,FP_COMP_OPS_EXE:X87,FP_COMP_OPS_EXE:MMX,FP_COMP_OPS_EXE:SSE_SINGLE_PRECISION,FP_COMP_OPS_EXE:SSE_DOUBLE_PRECISION");
    //}
  }

  /* Set processor-specific parameters
   * These are picked up during the report generation phase.
   */
  char papiex_report_arch_values[PATH_MAX] = "";
  if (is_intel_core_processor(hwinfo->vendor,hwinfo->cpuid_family,hwinfo->cpuid_model)) {
    PAPIEX_DEBUG("Found a Intel Core microarchitecture (Nehalem) processor.\n");
    sprintf(papiex_report_arch_values, "CPU=\"%s\",L2_LATENCY=10,L3_LATENCY=40,MEM_LATENCY=200,WORD_SIZE=8", hwinfo->model_string);
  }
  if ((strcmp(hwinfo->vendor_string, INTEL_VENDOR_STRING) == 0) && (strcmp(hwinfo->model_string, INTEL_ITANIUM2_MODEL_STRING)==0)) {
    PAPIEX_DEBUG("Found an Itanium 2 processor.\n");
    sprintf(papiex_report_arch_values, "CPU=\"Intel Itanium 2\",L2_LATENCY=5,L3_LATENCY=14,MEM_LATENCY=200,WORD_SIZE=8");
  }
  if (strlen(papiex_report_arch_values)) {
    const char* old_papiex_report_arch_values = getenv ("PAPIEX_REPORT_ARCH_VALUES");
    if (old_papiex_report_arch_values) 
      sprintf(papiex_report_arch_values,"%s:%s", papiex_report_arch_values, old_papiex_report_arch_values);
      setenv("PAPIEX_REPORT_ARCH_VALUES", papiex_report_arch_values, 1);
  }

  /* Test PAPI events for sanity and availability */
  /* Add the fixed up events to the list of options */
  if (strlen(events)) {
    strncpy(requested_events, events, sizeof(requested_events));
    if (!no_compat_check) {
      int rc = test_papi_events(events);
      PAPIEX_DEBUG("test_papi_events: returned %d\n", rc);
      if ((test_events_only || auto_stats || skip_unavailable_events) && (rc != PAPI_OK))
        make_compatible_eventset(events);
    }
    strncpy(successful_events, events, sizeof(successful_events));
    append_option (option_string, events);
  }
  if (no_compat_check) {
    PAPIEX_DEBUG("--no-compat-check set, so events not tested for compatibility\n");
  }

  /* Process the long options */
  if (no_mpi_gather) {
    append_option(option_string, "NO_MPI_GATHER");
  }
  if (no_summary_stats) {
    append_option(option_string, "NO_SUMMARY_STATS");
  }

  if (no_derived_stats) {
    append_option(option_string, "NO_DERIVED_STATS");
  }

#ifndef USE_MPI
  no_mpi_prof = 1;
#endif

  if (no_mpi_prof) {
    append_option(option_string, "NO_MPI_PROF");
  }

  if (no_io_prof) {
    append_option(option_string, "NO_IO_PROF");
  }

  if (no_threadsync_prof) {
    append_option(option_string, "NO_THREADSYNC_PROF");
  }

  if (no_scientific) {
    append_option(option_string, "NO_SCIENTIFIC");
  }

  if (!classic) {
    append_option(option_string, "NEXTGEN");
  }
  if (csv_output) {
    append_option(option_string, "CSV");
  }

  /* Construct argv for the fork/exec pair later on. */

  if (shell_dump == 0 && !test_events_only)
    {
      char **tmp_my_argv;
      int i;

      if (my_argv[optind] == NULL)
	{
	  PAPIEX_WARN ("no command given\n");
	  goto error;
	}

      tmp_my_argv =
	(char **) malloc ((my_argc - optind + 1) * sizeof (char *));
      for (i = optind; i < my_argc; i++)
	tmp_my_argv[i - optind] = strdup (my_argv[i]);
      tmp_my_argv[i - optind] = NULL;
      *cmd_argv = tmp_my_argv;
      *cmd = strdup (tmp_my_argv[0]);
    }

#ifdef HAVE_PAPI
  PAPI_shutdown ();
#endif

  return (option_string);
}

#ifdef HAVE_PAPI

/*
static int is_derived(PAPI_event_info_t *info)
{
  if (strlen(info->derived) == 0)
    return(0);
  else if (strcmp(info->derived,"NOT_DERIVED") == 0)
    return(0);
  else if (strcmp(info->derived,"DERIVED_CMPD") == 0)
    return(0);
  else
    return(1);
}
*/

static char *stringify_domain(int domain)
{
  static char output[256];
  output[0] = '\0';
  if (domain & PAPI_DOM_USER)
    strcat(output,"User");
  if (domain & PAPI_DOM_KERNEL)
    {
      append_option(output,"Kernel");
    }
  if (domain & PAPI_DOM_OTHER)
    {
      append_option(output,"Other");
    }
  if (domain & PAPI_DOM_SUPERVISOR)
    {
      append_option(output,"Supervisor");
    }
  return(output);
}

static char *stringify_all_domains(int domains)
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

static char *stringify_granularity(int granularity)
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

static char *stringify_all_granularities(int granularities)
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

/* This function presently adds available, PAPI preset events
 * to 'events'. For "known" architectures we could do better
 * and add native events in the future
 * 'events' can point to some pre-existing events, so please append
 * to it! Also note that the length of events string is maxlen.
 */
static void get_useful_events(char *events, int maxlen) {
  int len = strlen(events);

  /* For special architectures we use native events 
   * unless --use-papi-presets is set */

  if (!use_papi_presets) { 
    /* MIPS 74K */
    if ((strcmp(hwinfo->vendor_string, "MIPS")==0) && (strncmp(hwinfo->model_string, "74K",3)==0)) {
      PAPIEX_DEBUG("Found a MIPS 74K processor\n");
      if (strlen(events)) 
        strcat(events, ",");
      if (strlen(mips_74K_useful_events)+len >= maxlen-1) {
        PAPIEX_ERROR("Cannot fit all the useful events in the buffer\n"
                     "Please allocate a bigger buffer!\n");
        exit(2);
      } 
      strcat(events, mips_74K_useful_events);
      return;
    }

    /* SiCortex 9A */
    if ((strcmp(hwinfo->vendor_string, SC_VENDOR_STRING)==0) && (strcmp(hwinfo->model_string, SC_ICE9A_STRING)==0)) {
      PAPIEX_DEBUG("Found a SiCortex ICE9A processor\n");
      if (strlen(events)) 
        strcat(events, ",");
      if (strlen(sc_ice9A_useful_events)+len >= maxlen-1) {
        PAPIEX_ERROR("Cannot fit all the useful events in the buffer\n"
                     "Please allocate a bigger buffer!\n");
        exit(2);
      } 
      strcat(events, sc_ice9A_useful_events);
      return;
    }

    /* SiCortex 9B */
    if ((strcmp(hwinfo->vendor_string, SC_VENDOR_STRING)==0) && (strcmp(hwinfo->model_string, SC_ICE9B_STRING)==0)) {
      PAPIEX_DEBUG("Found a SiCortex ICE9B processor\n");
      if (strlen(events)) 
        strcat(events, ",");
      if (strlen(sc_ice9B_useful_events)+len >= maxlen-1) {
        PAPIEX_ERROR("Cannot fit all the useful events in the buffer\n"
                     "Please allocate a bigger buffer!\n");
        exit(2);
      } 
      strcat(events, sc_ice9B_useful_events);
      return;
    }

    int model = -1;
    if ((model=is_intel_core_processor(hwinfo->vendor,hwinfo->cpuid_family,hwinfo->cpuid_model))) {
      PAPIEX_DEBUG("Found a Intel Core microarchitecture (Nehalem) processor\n");
      const char* events_list = corei7_events;
      if (model == 44) {
        PAPIEX_DEBUG("This is actually the Westmere (model=44)\n");
        events_list = westmere_events;
      }
      if (strlen(events_list)+len >= maxlen-1) {
        PAPIEX_ERROR("Cannot fit all the useful events in the buffer\n"
                     "Please allocate a bigger buffer!\n");
        exit(2);
      } 
      strcat(events, events_list);
      return;
    }
  }  /* use_papi_presets */

  /* Default, for unknown architectures */
  if (strlen(papi_useful_presets)+len >= maxlen-1) {
    PAPIEX_ERROR("Cannot fit all the useful events in the buffer\n"
                 "Please allocate a bigger buffer!\n");
    exit(2);
  } 
  append_option(events, papi_useful_presets);
/*
  PAPI_event_info_t info;
  int i, j, l;
  int k = 0;
  l =0; j=0;
  i = 0 | PAPI_PRESET_MASK;
  j = 1;
  do {
       j++;
       if (PAPI_get_event_info(i, &info) == PAPI_OK) {
         l++;
         //if (info.count && !is_derived(&info)){
         if (info.count){
           if (strlen(events)) 
             strcat(events, ",");
           if (strlen(info.symbol)+len >= maxlen-1) {
             fprintf(stderr, "Cannot fit all the useful events in the buffer\n"
                             "Please allocate a big buffer!\n");
             exit(2);
           } 
           strcat(events, info.symbol);
           len += strlen(info.symbol) + 1; // 1 for the comma
           k++;
           if (k>=PAPIEX_MAX_COUNTERS) {
             fprintf(stderr, "* NOTICE: Reached the limit on MAX MULTIPLEX COUNTERS (%d)\n"
                             "Skipping addition of remaining events\n", PAPIEX_MAX_COUNTERS);
           }
         }
      }
  } while (PAPI_enum_event(&i, PAPI_ENUM_ALL) == PAPI_OK);
*/  

  return;
}

/*
static void _papiex_add_useful_events(char *eventnames[], int *eventcnt) {
  int i,k;
  PAPI_event_info_t info;
  i = PAPI_PRESET_MASK; k = 0;
  do {
       if (PAPI_get_event_info(i, &info) == PAPI_OK) {
         if (info.count == 1) {
           eventnames[*eventcnt] = strdup(info.symbol);
           k++;
           *eventcnt = *eventcnt + 1;
         }
      };
  } while (PAPI_enum_event(&i, PAPI_ENUM_ALL) == PAPI_OK);
  fprintf(stderr, "Added %d events for automatic monitoring\n", k);
  return;
}
*/

static void dump_substrate_info(FILE *output) 
{
  const PAPI_component_info_t *ss_info;
  ss_info = PAPI_get_component_info(0);
  if (ss_info != NULL) {
    if (ss_info->name != NULL && strlen(ss_info->name))
      fprintf(output, "%-30s: %s\n", "Substrate name",ss_info->name);

    if (ss_info->version != NULL && strlen(ss_info->version))
      fprintf(output, "%-30s: %s\n", "Substrate version",ss_info->version);

    if (ss_info->support_version != NULL && strlen(ss_info->support_version))
      fprintf(output, "%-30s: %s\n", "Support lib version",ss_info->support_version);

    if (ss_info->kernel_version != NULL && strlen(ss_info->kernel_version))
      fprintf(output, "%-30s: %s\n", "Kernel driver version",ss_info->kernel_version);
    fprintf(output, "%-30s: %d\n","Num. counters",ss_info->num_cntrs);
    fprintf(output, "%-30s: %d\n","Num. preset events",ss_info->num_preset_events);
    fprintf(output, "%-30s: 0x%x (%s)\n","Default domain",ss_info->default_domain,stringify_all_domains(ss_info->default_domain)); 
    fprintf(output, "%-30s: 0x%x (%s)\n","Available domains",ss_info->available_domains,stringify_all_domains(ss_info->available_domains));
    fprintf(output, "%-30s: 0x%x (%s)\n","Default granularity",ss_info->default_granularity,stringify_granularity(ss_info->default_granularity));
    fprintf(output, "%-30s: 0x%x (%s)\n","Available granularities",ss_info->available_granularities,stringify_all_granularities(ss_info->available_granularities));
#ifdef PAPI_DEF_MPX_NS
    //fprintf(output, "%-30s: %d\n","Itimer sig",ss_info->itimer_sig); 
    //fprintf(output, "%-30s: %d\n","Itimer num",ss_info->itimer_num);
    //fprintf(output, "%-30s: %d\n","Itimer ns",ss_info->itimer_ns); 
    //fprintf(output, "%-30s: %d\n","Itimer res us",ss_info->itimer_res_ns); 
#else
    fprintf(output, "%-30s: %d\n","Multiplex timer sig",ss_info->multiplex_timer_sig); 
    fprintf(output, "%-30s: %d\n","Multiplex timer num",ss_info->multiplex_timer_num);
#endif
    fprintf(output, "%-30s: %d\n","Hardware intr sig",ss_info->hardware_intr_sig);  
    //fprintf(output, "%-30s: %d\n","Opcode match width",ss_info->opcode_match_width);
    fprintf(output, "%-30s: %s\n","Hardware intr",ss_info->hardware_intr==0 ? "No" : "Yes");
    fprintf(output, "%-30s: %s\n","Precise intr",ss_info->precise_intr==0 ? "No" : "Yes"); 
    fprintf(output, "%-30s: %s\n","Posix1b timers",ss_info->posix1b_timers==0 ? "No" : "Yes");
    fprintf(output, "%-30s: %s\n","Kernel profile",ss_info->kernel_profile==0 ? "No" : "Yes");
    fprintf(output, "%-30s: %s\n","Kernel multiplex",ss_info->kernel_multiplex==0 ? "No" : "Yes");
    //fprintf(output, "%-30s: %d\n","Data address range",ss_info->data_address_range);
    //fprintf(output, "%-30s: %d\n","Instr address range",ss_info->instr_address_range);
    fprintf(output, "%-30s: %s\n","Fast counter read",ss_info->fast_counter_read==0 ? "No" : "Yes"); 
    fprintf(output, "%-30s: %s\n","Fast real timer",ss_info->fast_real_timer==0 ? "No" : "Yes");
    fprintf(output, "%-30s: %s\n","Fast virtual timer",ss_info->fast_virtual_timer==0 ? "No" : "Yes");
    //fprintf(output, "%-30s: %s\n","Data address_smpl",ss_info->data_address_smpl==0 ? "No" : "Yes");
    //fprintf(output, "%-30s: %s\n","Branch tracing",ss_info->branch_tracing==0 ? "No" : "Yes");
    //fprintf(output, "%-30s: %s\n","TLB address smpl",ss_info->tlb_address_smpl==0 ? "No" : "Yes");
    //fprintf(output, "%-30s: %s\n","Grouped cntrs",ss_info->cntr_groups==0 ? "No" : "Yes"); 
  }
  else {
    fprintf(output, "Error obtaining substrate information from PAPI\n"
            "PAPI_get_substrate_info returned NULL\n");
  }
  return; 
}

/* events is a comma-separated list of PAPI events */
static int test_papi_events(const char *events) {
  PAPIEX_DEBUG("Verifying %s for availability and compatibility\n", events);

  int retval = 0;
  static char *eventnames[PAPIEX_MAX_COUNTERS];
  static int eventcodes[PAPIEX_MAX_COUNTERS];

  memset(eventnames,0x0,sizeof(char *)*PAPIEX_MAX_COUNTERS);
  memset(eventcodes,0x0,sizeof(int)*PAPIEX_MAX_COUNTERS);
  char events2[PATH_MAX];
  strcpy(events2, events);
  int i = 0;
  char *tmp;
  if (strlen(events)) {
    tmp = strtok(events2, ",");
    do {
      int eventcode;
      retval = PAPI_event_name_to_code(tmp,&eventcode);
      if (retval != PAPI_OK) {
        if (skip_unavailable_events) {
          PAPIEX_DEBUG("Could not map event to code for %s\n", tmp);
          if (strlen(unavailable_events))
            strcat(unavailable_events, ",");
          strcat(unavailable_events, tmp);
          PAPIEX_PAPI_DEBUG("PAPI_event_name_to_code",retval);
        }
        else {
          PAPIEX_WARN("Could not map event to code for %s\n", tmp);
          PAPIEX_PAPI_ERROR("PAPI_event_name_to_code",retval);
        }
        could_not_add_some_events = 1;
        if (test_events_only)
          continue;
        return retval;
      }
      eventnames[i] = strdup(tmp);
      eventcodes[i] = eventcode;
      //PAPIEX_DEBUG("added event %s with code %d to eventset\n", tmp, eventcode);
      i++;
      if (i == PAPIEX_MAX_COUNTERS)
     	PAPIEX_ERROR("Exceeded maximum number of events allowed by papiex (%d).\n",PAPIEX_MAX_COUNTERS);
    } while ((tmp=strtok(NULL, ",")));

    if (multiplex) {
      PAPIEX_DEBUG("Enabling multiplexing in the PAPI library\n");
      retval = PAPI_multiplex_init();
      if (retval != PAPI_OK) {
        PAPIEX_PAPI_ERROR("PAPI_multiplex_init",retval);
        return retval;
      }
    }
    int eventcnt = i;
    int eventset = PAPI_NULL;
    retval = PAPI_create_eventset(&eventset);
    if (retval != PAPI_OK) {
      if (!skip_unavailable_events)
        PAPIEX_PAPI_ERROR("PAPI_create_eventset",retval);
      return retval;
    }

    if (multiplex) {
      retval = PAPI_assign_eventset_component(eventset, 0);
      if (retval != PAPI_OK) {
        PAPIEX_PAPI_ERROR("PAPI_assign_eventset_component",retval);
        return retval;
      }
      retval = PAPI_set_multiplex(eventset);
      PAPIEX_DEBUG("Enabling multiplexing on the eventset\n");
      if (retval != PAPI_OK) {
        //if (!auto_stats)
        PAPIEX_PAPI_ERROR("PAPI_set_multiplex",retval);
        return retval;
      }
    }
    PAPIEX_DEBUG("Adding eventset (while testing for compatibility)\n");
    retval = PAPI_add_events(eventset, eventcodes, eventcnt);
    if (retval < PAPI_OK) {
      could_not_add_some_events = 1;
      if (!skip_unavailable_events)
        PAPIEX_PAPI_ERROR("PAPI_add_events",retval);
      return retval;
    }
    if (retval != PAPI_OK) {
      char str[PAPI_MAX_STR_LEN];
      sprintf(str,"PAPI_add_events(%s)",eventnames[retval]);
      if (skip_unavailable_events) {
        could_not_add_some_events = 1;
        if (strlen(incompatible_events))
          strcat(incompatible_events, ",");
        strcat(incompatible_events, eventnames[retval]);
        if (!auto_stats)
          PAPIEX_PAPI_WARN(str,PAPI_add_event(eventset,eventcodes[retval]));
      }
      else {
        PAPIEX_PAPI_ERROR(str,PAPI_add_event(eventset,eventcodes[retval]));
      }
      return retval;
    }
    if (retval==PAPI_OK && !could_not_add_some_events) 
      PAPIEX_DEBUG("Verified events for compatibility: %s\n", events);
  }
  return (retval || could_not_add_some_events);
}

/* events is a comma-separated list of PAPI events */
/* It CAN be modified to produce a set of compatible events */
static int make_compatible_eventset(char *events) {
  PAPIEX_DEBUG("Making a set of compatible events from:  %s\n", events);

  int retval = 0;
  static char *eventnames[PAPIEX_MAX_COUNTERS];
  static int eventcodes[PAPIEX_MAX_COUNTERS];
  unavailable_events[0] = '\0';
  incompatible_events[0] = '\0';

  memset(eventnames,0x0,sizeof(char *)*PAPIEX_MAX_COUNTERS);
  memset(eventcodes,0x0,sizeof(int)*PAPIEX_MAX_COUNTERS);
  char events2[PATH_MAX];
  char comp_events[PATH_MAX] = "";
  strcpy(events2, events);
  int i = 0;
  char *tmp;
  if (multiplex) {
    PAPIEX_DEBUG("Enabling multiplexing in the PAPI library\n");
    retval = PAPI_multiplex_init();
    if (retval != PAPI_OK)
      PAPIEX_PAPI_ERROR("PAPI_multiplex_init",retval);
  }

  if (strlen(events2)) {
    tmp = strtok(events2, ",");
    do {
      int eventcode;
      PAPIEX_DEBUG("Checking %s for compatibility\n", tmp);
      retval = PAPI_event_name_to_code(tmp,&eventcode);
      if (retval != PAPI_OK) {
        PAPIEX_WARN("Could not map event to code for %s. Skipping it\n", tmp);
        if (strlen(unavailable_events))
          strcat(unavailable_events, ",");
        strcat(unavailable_events, tmp);
        continue;
      }
      if (PAPI_query_event(eventcode) != PAPI_OK) {
        if (strlen(incompatible_events))
          strcat(incompatible_events, ",");
        strcat(incompatible_events, tmp);
        if (!auto_stats)
          PAPIEX_WARN("Event %s cannot be counted on this arch, skipping it\n", tmp);
        continue;
      }

      eventnames[i] = strdup(tmp);
      eventcodes[i] = eventcode;
      i++;
      int eventcnt = i;
      int eventset = PAPI_NULL;
      retval = PAPI_create_eventset(&eventset);
      if (retval != PAPI_OK)
        PAPIEX_PAPI_ERROR("PAPI_create_eventset",retval);

      if (multiplex) {
        retval = PAPI_assign_eventset_component(eventset, 0);
        if (retval != PAPI_OK) {
          PAPIEX_PAPI_ERROR("PAPI_assign_eventset_component",retval);
          return retval;
        }
        retval = PAPI_set_multiplex(eventset);
        if (retval != PAPI_OK)
          PAPIEX_PAPI_ERROR("PAPI_set_multiplex",retval);
      }
      retval = PAPI_add_events(eventset, eventcodes, eventcnt);
      if (retval != PAPI_OK) {
        if (!auto_stats)
          PAPIEX_WARN("Could not add event %s, so skipping it\n", tmp);
        if (strlen(incompatible_events))
          strcat(incompatible_events, ",");
        strcat(incompatible_events, tmp);
        i--;
        could_not_add_some_events = 1;
      }
      else {
        PAPIEX_DEBUG("%s is compatible\n", tmp);
        append_option(comp_events, tmp);
      }
      PAPI_cleanup_eventset(eventset);
      PAPI_destroy_eventset(&eventset);
    } while ((tmp=strtok(NULL, ",")));

    strcpy(events, comp_events);
    if (retval==PAPI_OK)
      PAPIEX_DEBUG("Verified events for compatibility: %s\n", events);
  }
  return (retval);
}
#endif

void dump_shell_vars(char *optstr, char *outstr, char *preload_env, char *preload_val, char *ldpath_env, char *ldpath_val)
{
  char *basesh;
  char *sh = getenv ("SHELL");
  int not_csh = 0;
  
  if ((sh == NULL) || (strlen (sh) == 0))
    {
    bail:
      PAPIEX_ERROR ("Error: no valid SHELL environment variable\n");
      exit (1);
    }
  basesh = basename (sh);
  if ((basesh == NULL) || (strlen (basesh) == 0))
    goto bail;
  
  if ((strcmp (basesh, "bash") == 0) || (strcmp (basesh, "sh") == 0) ||
      (strcmp (basesh, "zsh") == 0) || (strcmp (basesh, "ksh") == 0) ||
      (strcmp (basesh, "ash") == 0))
    not_csh = 1;
  
  if (getenv(PAPIEX_DEFAULT_ARGS)) 
    printf ("%s %s%s\"%s\";\n", (not_csh ? "export" : "setenv"), PAPIEX_DEFAULT_ARGS,
	    (not_csh ? "=" : " "), getenv(PAPIEX_DEFAULT_ARGS));

  if (getenv(PAPIEX_ENV)) 
    printf ("%s %s%s%s;\n", (not_csh ? "export" : "setenv"), PAPIEX_ENV,
	    (not_csh ? "=" : " "), optstr);

  if (getenv(PAPIEX_OUTPUT_ENV))
    printf ("%s %s%s%s;\n", (not_csh ? "export" : "setenv"),
	    PAPIEX_OUTPUT_ENV, (not_csh ? "=" : " "), outstr);
  
  if (getenv("PAPIEX_DEBUG"))
    printf ("%s %s%s%s;\n", (not_csh ? "export" : "setenv"),
	    "PAPIEX_DEBUG", (not_csh ? "=" : " "), "1");

  if (getenv("MONITOR_DEBUG"))
    printf ("%s %s%s%s;\n", (not_csh ? "export" : "setenv"),
	    "MONITOR_DEBUG", (not_csh ? "=" : " "), "1");

  if (getenv("MONITOROPTIONS"))
    printf ("%s %s%s%s;\n", (not_csh ? "export" : "setenv"),
	    "MONITOROPTIONS", (not_csh ? "=" : " "), "1");
  
  if (getenv(preload_env))
    printf ("%s %s%s\"%s\";\n", (not_csh ? "export" : "setenv"),
	    preload_env, (not_csh ? "=" : " "), preload_val);

  if (getenv(ldpath_env))
    printf ("%s %s%s%s;\n", (not_csh ? "export" : "setenv"), ldpath_env,
	    (not_csh ? "=" : " "), getenv(ldpath_env));
}

int
main (int argc, char **argv)
{
  static char tmpstr[PATH_MAX] = "";
  static char tmpstr2[PATH_MAX] = "";
  static char outstr[PATH_MAX] = "";
/*  char ldlp[PATH_MAX] = ""; */
#ifdef HAVE_PAPI
  char papilib[PATH_MAX] = "";
#endif
  char monitorlib[PATH_MAX] = "";
  char preload_env[PATH_MAX] = "LD_PRELOAD";
  char preload_sep = ' ';
  char ldpath_env[PATH_MAX] = "LD_LIBRARY_PATH";
  char ldpath_sep = ':';
  char *cmd = NULL;
  char **cmd_argv = NULL;
  char *optstr = NULL;
  char new_ld_path[PATH_MAX];


  /* Add default arguments if any */
  int myargc = argc;
  char** myargv = argv;
  char* default_args = getenv(PAPIEX_DEFAULT_ARGS);
  if ((strcmp(basename(argv[0]), "papiex")==0) && default_args) {
    myargv = malloc(sizeof(char *)*256);
    if (myargv == NULL) {
      fprintf(stderr, "malloc failed. Quitting now!\n");
      exit(1);
    }
    myargv[0] = argv[0];
    myargc = 1;
    char *tmp =strtok(strdup(default_args), " ");
    do {
      myargv[myargc] = tmp;
      PAPIEX_DEBUG("prepending option %s", tmp);
      myargc++;
    } while ((tmp=strtok(NULL," ")));
    int i;
    for (i=1;i<argc;i++)
      myargv[myargc++] = argv[i];
    myargv[myargc] = (char*) NULL;
  }
  /* for debugging
    int j;
    for (j=0; j<myargc; j++)
      fprintf(stderr, "myargv[%d] = %s\n", j, myargv[j]);
  */
  optstr =
    parse_args (myargc, myargv, &cmd, &cmd_argv, preload_env, &preload_sep, ldpath_env, &ldpath_sep,
		outstr);
  if ((optstr == NULL) || (strlen (optstr) == 0))
    sprintf (tmpstr, "%s= ", PAPIEX_ENV);
  else
    sprintf (tmpstr, "%s=%s", PAPIEX_ENV, optstr);

#ifdef HAVE_MONITOR
  sprintf (monitorlib, "libmonitor.so");
#endif

#ifdef HAVE_PAPI
  sprintf (papilib, "libpapi.so%c", preload_sep);
#endif

#ifdef USE_MPI
  if (!no_mpi_prof) {
    if (strlen (tmpstr2))
      strncat (tmpstr2, &preload_sep, 1);
    strcat(tmpstr2, MPI_PROFILE_LIB);
  }
#endif

  if (!no_io_prof) {
    if (strlen (tmpstr2))
      strncat (tmpstr2, &preload_sep, 1);
    strcat(tmpstr2, IO_PROFILE_LIB);
  }

  if (!no_threadsync_prof) {
    if (strlen (tmpstr2))
      strncat (tmpstr2, &preload_sep, 1);
    strcat(tmpstr2, THREADSYNC_PROFILE_LIB);
  }

  if (gcc_prof)
    {
      if (strlen(tmpstr2))
	strncat (tmpstr2, &preload_sep, 1);
      strcat(tmpstr2, "libpapiexgcc.so");
    }

  if (pathscale_prof)
    {
      if (strlen(tmpstr2))
	strncat (tmpstr2, &preload_sep, 1);
      strcat(tmpstr2, "libpapiexpathscale.so");
    }

  /* papiex lib */
  if (strlen (tmpstr2))
    strncat (tmpstr2, &preload_sep, 1);
  strcat(tmpstr2, "libpapiex.so");

  /* monitor lib */
  if (strlen (tmpstr2))
    strncat (tmpstr2, &preload_sep, 1);
  strcat(tmpstr2, monitorlib);

  /* was there already something to be preloaded? */
  if (getenv(preload_env)) {
    if (strlen (tmpstr2))
      strncat (tmpstr2, &preload_sep, 1);
    strcat(tmpstr2, getenv(preload_env));
  }


  /* Do we need to modify LD_LIBRARY_PATH? */
  if (!no_ld_library_path) {
     new_ld_path[0] = '\0';
#ifdef PAPIEX_LD_LIBRARY_PATH
     sprintf(new_ld_path, "%s", PAPIEX_LD_LIBRARY_PATH);
     new_ld_paths_added = 1;
#endif
    
    /* if the environment variable PAPIEX_LD_LIBRARY_PATH is
     * is set, then that takes precedence,
     */
     char *papiex_ld_path_env = getenv(ENV_PAPIEX_LD_LIBPATH);
     if (papiex_ld_path_env != NULL) {
       sprintf(new_ld_path, "%s", papiex_ld_path_env);
       new_ld_paths_added = 1;
     }

     /* We prepend the path we just created to the LD_LIBRARY_PATH
      * if it's already set. Otherwise, we initialize it
      */
      if (strlen(new_ld_path)) {
        if (getenv(ldpath_env)) {
          strncat(new_ld_path, &ldpath_sep, 1);
          strcat(new_ld_path, getenv(ldpath_env));
        }
        setenv (ldpath_env, new_ld_path, 1);
      }
  } /* no_ld_library_path */

  if (strlen(tmpstr2))
    setenv (preload_env, tmpstr2, 1);
  setenv (PAPIEX_ENV, optstr, 1);
  setenv (PAPIEX_OUTPUT_ENV, outstr, 1);
  if (debug > 1) {
    setenv ("MONITOR_DEBUG","1",1);
  }
  setenv ("MONITOR_OPTIONS","SIGINT",1);
  
  if ((debug >= 1) || (shell_dump))
    dump_shell_vars(optstr,outstr,preload_env,tmpstr2,ldpath_env,new_ld_path);

  if (could_not_add_some_events && !auto_stats) {
    PAPIEX_WARN("Could not add one or more events for measurement\n");
  }
  if (test_events_only) {
    if (!strlen(requested_events)) {
      PAPIEX_ERROR("--test needs one or more events to be tested with -e\n");
    }
    fprintf(stderr, "Accepted Event(s): %s\n", successful_events);
    if (!could_not_add_some_events) {
      fprintf(stderr, "All events are available and compatible\n");
      exit(0);
    }
    else {
      if (strlen(unavailable_events))
        fprintf(stderr, "Unavailable Event(s): %s\n", unavailable_events);
      if (strlen(incompatible_events))
        fprintf(stderr, "Incompatible/Hardware-Unsupported Event(s): %s\n", incompatible_events);
      exit(1);
    }
  }

  if (shell_dump || test_events_only)
    exit(0);

  if (fork () == 0)
    {
      if (execvp (cmd, cmd_argv) == -1)
	{
          /* check if adding "." to PATH fixes this? */
          char path[PATH_MAX] = ".";
          if (getenv("PATH"))
            sprintf(path, "%s%c.", getenv("PATH"), ldpath_sep);
          setenv("PATH", path, 1);
          if (execvp (cmd, cmd_argv) == -1) {
	    PAPIEX_ERROR ("Error exec'ing %s: %s\n", cmd, strerror (errno));
	    exit (1);
          }
	}
      exit(0);
    }
  else
    {
      int status = 0;
      wait (&status);
      exit (WEXITSTATUS (status));
    }
}
