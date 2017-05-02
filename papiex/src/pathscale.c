#include "papiex_internal.h"

/* This interface is documented in:
   /net/sicortex2/kits/pathscale/opteron-kits/pathscale-compiler-sources-2.2.1-2005.08.05/kpro64/instrumentation/libinstr/profile_interface.h
   /net/sicortex2/kits/pathscale/opteron-kits/pathscale-compiler-sources-2.2.1-2005.08.05/kpro64/instrumentation/libinstr/profile_interface.cxx */

static int papiex_pathscale_enable = 0;
static __thread int total_points = 1;

void papiex_pathscale_set_gate(int i)
{
  LIBPAPIEX_DEBUG("%d\n",i);
  switch(i)
    {
    case PATHSCALE_LOOPS:
    case PATHSCALE_FUNCS:
    case PATHSCALE_ALL:
    case PATHSCALE_NONE:
      papiex_pathscale_enable = i;
      break;
    default:
      LIBPAPIEX_DEBUG("Unknown gate value %d, assuming all.\n",i);
      papiex_pathscale_enable = PATHSCALE_ALL;
      break;
    }
}

void papiex_pathscale_thread_init(papiex_perthread_data_t *thread_data)
{
  LIBPAPIEX_DEBUG("%p\n",thread_data);
}

// One time initialization
void __profile_init(char *fname, int phase_name, int unique_name)
{
  LIBPAPIEX_DEBUG("%s,%d,%d\n",fname,phase_name,unique_name);
}
// PU level initialization to gather profile information for the PU.
// We call atexit during the first call to this routine to ensure
// that at exit we remember to destroy the data structures and dump
// profile information.
// Also, during the first call, a profile handle for the PU is created.
// During subsequent calls, teh PC address of the PU is used to access
// a hash table and return the profile handle that was created during the 
// first call.

void *__profile_pu_init(char *fname, char *pu_name, long pc, int pusize, int checksum)
{
  pathscale_pu_t *pu_handle;
  LIBPAPIEX_DEBUG("%s,%s,0x%lx,%d,%d\n",fname,pu_name,pc,pusize,checksum);

  pu_handle = (pathscale_pu_t *)malloc(sizeof(pathscale_pu_t));
  if (pu_handle == NULL)
    return(NULL);
  memset(pu_handle,0,sizeof(pathscale_pu_t));
  if (fname)
    pu_handle->fname = strdup(fname);
  if (pu_name)
    pu_handle->pu_name = strdup(pu_name);
  pu_handle->num_loops = 0;
  pu_handle->num_calls = 0;
  pu_handle->loops = NULL;
  pu_handle->calls = NULL;
  pu_handle->pusize = pusize;
  pu_handle->pc = pc; 
  pu_handle->checksum = checksum;
  return(pu_handle);
}
 
// For a PU, initialize the data structures that maintain 
// invokation profile information.

void
__profile_invoke_init(void *pu_handle, INT32 num_invokes)
{
  LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,num_invokes);
}

// Gather profile information for a conditional invoke

void
__profile_invoke(void *pu_handle, INT32 invoke_id)
{
  LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,invoke_id);
}

// For a PU, initialize the data structures that maintain 
// conditional branch profile information.

void
__profile_branch_init(void *pu_handle, INT32 num_branches)
{
  LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,num_branches);
}

// Gather profile information for a conditional branch

void
__profile_branch(void *pu_handle, INT32 branch_id, bool taken)
{
  LIBPAPIEX_DEBUG("%p,%d,%d\n",pu_handle,branch_id,taken);
}

// For a PU, initialize the data structures that maintain 
// switch profile information.

void
__profile_switch_init(void *pu_handle,
		      INT32 num_switches,    INT32 *switch_num_targets,
		      INT32 num_case_values, INT64 *case_values)
{
  LIBPAPIEX_DEBUG("%p,%d,%p,%d,%p\n",pu_handle,num_switches,switch_num_targets,num_case_values,case_values);
}

// Gather profile information for an Switch

void
__profile_switch(void *pu_handle, INT32 switch_id, INT32 target,
		   INT32 num_targets)
{
  LIBPAPIEX_DEBUG("%p,%d,%d,%d\n",pu_handle,switch_id,target,num_targets);
}

// For a PU, initialize the data structures that maintain 
// compgoto profile information.

void
__profile_compgoto_init(void *pu_handle, INT32 num_compgotos,
			INT32 *compgoto_num_targets)
{
  LIBPAPIEX_DEBUG("%p,%d,%p\n",pu_handle,num_compgotos,compgoto_num_targets);
}

// Gather profile information for an Compgoto

void
__profile_compgoto(void *pu_handle, INT32 compgoto_id, INT32 target,
		   INT32 num_targets)
{
  LIBPAPIEX_DEBUG("%p,%d,%d,%d\n",pu_handle,compgoto_id,target,num_targets);
}

// For a PU, initialize the data structures that maintain 
// loop profile information.

void 
__profile_loop_init(void *pu_handle, INT32 num_loops)
{
  if (papiex_pathscale_enable & PATHSCALE_LOOPS)
    {
      int i, retval;
      void *tmp = NULL;
      papiex_perthread_data_t *thread_data = NULL;
      pathscale_pu_t *pu = pu_handle;
      LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,num_loops);
      retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
      if (retval != PAPI_OK)
	{
	  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
	  return;
	}
      thread_data = (papiex_perthread_data_t *)tmp;
      if (thread_data == NULL)
	{
	  LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.\n");
	  return;
	}
      if (total_points + num_loops > thread_data->max_caliper_entries)
	{
	  LIBPAPIEX_DEBUG("Not enough calipers for %d more loops.\n",num_loops);
	  return;
	}
      if ((pu->loops = realloc(pu->loops,(pu->num_loops+num_loops)*sizeof(int))) == NULL)
	{
	  fprintf(stderr,"pathscale.c: malloc of %d loops failed.\n",pu->num_loops*num_loops);
	  return;
	}
      PAPI_lock(PAPI_LOCK_USR1);
      for (i=pu->num_loops;i<pu->num_loops+num_loops;i++)
	{
	  pu->loops[i] = -(total_points+i);
	  LIBPAPIEX_DEBUG("Loop[%d] = point %d\n",i,(total_points+i));
	}
      total_points += num_loops;
      pu->num_loops += num_loops;
      PAPI_unlock(PAPI_LOCK_USR1);
      LIBPAPIEX_DEBUG("total_points now %d, num_loops now %d\n",total_points,pu->num_loops);
    }
}

// Gather profile information for a loop entry

void
__profile_loop(void *pu_handle, INT32 loop_id)
{
  if (papiex_pathscale_enable & PATHSCALE_LOOPS)
    {
      pathscale_pu_t *pu = pu_handle;
      if (loop_id >= pu->num_loops)
	return;
      if (pu->loops[loop_id] < 0)
	{
	  char label[MAX_LABEL_SIZE];
	  unsigned long pc;
	  pc = (unsigned long)__builtin_return_address(0);
	  LIBPAPIEX_DEBUG("%p,%d from PC 0x%lx\n",pu_handle,loop_id,pc);
	  sprintf(label,"L%d:%p",loop_id,(void *)pc);
	  pu->loops[loop_id] = -pu->loops[loop_id];
	  papiex_start(pu->loops[loop_id],label);
	}
      else
	papiex_start(pu->loops[loop_id],NULL);
    }
}

// Gather profile information from a Loop Iteration

void
__profile_loop_iter(void *pu_handle, INT32 loop_id) 
{
  if (papiex_pathscale_enable & PATHSCALE_LOOPS)
    {
      pathscale_pu_t *pu = pu_handle;
      LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,loop_id);
      if (loop_id >= pu->num_loops)
	return;
      papiex_accum(pu->loops[loop_id]);
    }
}

// For a PU, initialize the data structures that maintain
// call profiles.

void 
__profile_call_init(void *pu_handle, int num_calls)
{
  if (papiex_pathscale_enable & PATHSCALE_FUNCS)
    {
      int i, retval;
      void *tmp = NULL;
      papiex_perthread_data_t *thread_data = NULL;
      pathscale_pu_t *pu = pu_handle;
      LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,num_calls);
      retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
      if (retval != PAPI_OK)
	{
	  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
	  return;
	}
      thread_data = (papiex_perthread_data_t *)tmp;
      if (thread_data == NULL)
	{
	  LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.\n");
	  return;
	}
      if (total_points + num_calls > thread_data->max_caliper_entries)
	{
	  LIBPAPIEX_DEBUG("Not enough calipers for %d more calls.\n",num_calls);
	  return;
	}
      if ((pu->calls = realloc(pu->calls,(pu->num_calls+num_calls)*sizeof(int))) == NULL)
	{
	  fprintf(stderr,"pathscale.c: malloc of %d calls failed.\n",pu->num_calls*num_calls);
	  return;
	}
      PAPI_lock(PAPI_LOCK_USR1);
      for (i=pu->num_calls;i<pu->num_calls+num_calls;i++)
	{
	  pu->calls[i] = -(total_points+i);
	  LIBPAPIEX_DEBUG("Call[%d] = point %d\n",i,(total_points+i));
	}
      total_points += num_calls;
      pu->num_calls += num_calls;
      PAPI_unlock(PAPI_LOCK_USR1);
      LIBPAPIEX_DEBUG("total_points now %d, num_calls now %d\n",total_points,pu->num_calls);
    }
}

// For a PU, initialize the data structures that maintain
// icall profiles.

void 
__profile_icall_init(void *pu_handle, int num_icalls)
{
  LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,num_icalls);
}

// Gather the entry count for this call id

void 
__profile_call_entry(void *pu_handle, int call_id)
{
  if (papiex_pathscale_enable & PATHSCALE_FUNCS)
    {
      pathscale_pu_t *pu = pu_handle;
      if (call_id >= pu->num_calls)
	return;
      else if (pu->calls[call_id] < 0)
	{
	  char label[MAX_LABEL_SIZE];
	  unsigned long pc;
	  pc = (unsigned long)__builtin_return_address(0);
	  LIBPAPIEX_DEBUG("%p,%d from PC 0x%lx\n",pu_handle,call_id,pc);
	  sprintf(label,"C%d:%p",call_id,(void *)pc);
	  pu->calls[call_id] = -pu->calls[call_id];
	  papiex_start(pu->calls[call_id],label);
	}
      else
	papiex_start(pu->calls[call_id],NULL);
    }
}

extern void __profile_value_init(void *pu_handle, int num_values)
{
}
extern void __profile_value(void * pu_handle, int inst_id, long long value)
{
}
extern void __profile_value_fp_bin_init(void *pu_handle, int num_values)
{
}
extern void __profile_value_fp_bin(void * pu_handle, int inst_id, double value_fp_0, double value_fp_1 )
{
}

// Gather the exit count for this call id

void 
__profile_call_exit(void *pu_handle, int call_id)
{
  pathscale_pu_t *pu = pu_handle;
  if (papiex_pathscale_enable & PATHSCALE_FUNCS)
    {
      LIBPAPIEX_DEBUG("%p,%d\n",pu_handle,call_id);
      if (call_id >= pu->num_calls)
	return;
      papiex_stop(pu->calls[call_id]);
    }
}

void
__profile_icall(void * pu_handle, int icall_id, void * called_fun_address)
{
  LIBPAPIEX_DEBUG("%p,%d,%p\n",pu_handle,icall_id,called_fun_address);
}

// At exit processing to destroy data structures and dump profile
// information.

void __profile_finish(void)
{
  LIBPAPIEX_DEBUG("\n");
}

void papiex_pathscale_thread_shutdown(papiex_perthread_data_t *thread_data)
{
  LIBPAPIEX_DEBUG("%p\n",thread_data);
}

