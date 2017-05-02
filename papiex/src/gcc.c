/* The hashing grossness in this file is necessary because papiex currently
   has integer caliper points for each thread. There is a fixed number of them.
   Those must be mapped to addresses for auto-instrumentation. */

#include "papiex_internal.h"

static int papiex_gcc_enable = 0;

void papiex_gcc_set_gate(int i)
{
  LIBPAPIEX_DEBUG("%d\n",i);
  papiex_gcc_enable = i;
}

void papiex_gcc_thread_init(papiex_perthread_data_t *thread_data)
{
  LIBPAPIEX_DEBUG("%p\n",thread_data);
}

/** According to gcc documentation: called upon function entry */
void
__cyg_profile_func_enter(void *this_fn, void *call_site)
{
  if (!papiex_gcc_enable)
    return;

  LIBPAPIEX_DEBUG("Function enter %p from %p.\n",this_fn,call_site);
  {
  int retval;
  ENTRY e, *ep = NULL;
  struct hsearch_data *tab = NULL;
  papiex_perthread_data_t *thread_data = NULL;
  void *tmp = NULL;
  char key[ULTOHEXSTR_SZ];
  char label[MAX_LABEL_SIZE];
  int point;

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
  
  memset(&e,0,sizeof(e));
  tab = &thread_data->htab;
  if (tab->size == 0)
    {
      memset(&thread_data->htab,0x0,sizeof(thread_data->htab));
      if (hcreate_r(thread_data->max_caliper_entries,&thread_data->htab) < 0)
	{
	  LIBPAPIEX_ERROR("hcreate_r(%d) failed. %s\n",thread_data->max_caliper_entries,strerror(errno));
	  return;
	}
      LIBPAPIEX_DEBUG("Hash table of %d entries initialized.\n",thread_data->max_caliper_entries);
      tab = &thread_data->htab;
    }

  ultohexstr((unsigned long)this_fn,key);
  e.key = key;
  LIBPAPIEX_DEBUG("Looking up function 0x%s in hash table at %p.\n",e.key,tab);
  retval = hsearch_r(e,FIND,&ep,tab);
  if (retval)
    {
      if (ep == NULL)
	{
	  LIBPAPIEX_ERROR("hsearch_r(FIND,%s) succeeded, but returned NULL.\n",e.key);
	  return;
	}
      point = (int) ep->data;
      LIBPAPIEX_DEBUG("Function 0x%s found, point %d.\n",e.key,point);
      papiex_start(point,NULL);
      return;
    }
  else if ((retval == 0) && (errno != ESRCH))
    {
      LIBPAPIEX_ERROR("hsearch_t(FIND,%s) failed. %s\n",e.key,strerror(errno));
      return;
    }
  else
    {
      LIBPAPIEX_DEBUG("Function 0x%s not found.\n",e.key);
      if (thread_data->max_caliper_used+1 >= thread_data->max_caliper_entries)
	{
	  LIBPAPIEX_DEBUG("Not enough space for caliper point %d.\n",thread_data->max_caliper_used+1);
	  return;
	}
      sprintf(label,":%p",this_fn);
      
      point = thread_data->max_caliper_used+1;
      e.key = strdup(key);
      e.data = (void *)point;
      LIBPAPIEX_DEBUG("Entering function 0x%s as point %d.\n",e.key,point);
      retval = hsearch_r(e,ENTER,&ep,tab);
      if (retval == 0)
	{
	  LIBPAPIEX_ERROR("hsearch_r(ENTER,%s) failed. %s\n",e.key,strerror(errno));
	  return;
	}
      if (ep == NULL)
	{
	  LIBPAPIEX_ERROR("hsearch_t(ENTER,%s) succeeded, but returned NULL.\n",e.key);
	  return;
	}
      papiex_start(point,label);
    }
  }
}

/** According to gcc documentation: called upon function exit */
void
__cyg_profile_func_exit(void *this_fn, void *call_site)
{
  if (!papiex_gcc_enable)
    return;

  LIBPAPIEX_DEBUG("Function exit %p from %p.\n",this_fn,call_site);
  {
  int retval;
  ENTRY e, *ep = NULL;
  struct hsearch_data *tab = NULL;
  papiex_perthread_data_t *thread_data = NULL;
  void *tmp = NULL;
  char key[ULTOHEXSTR_SZ];
  int point;

  retval = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
  if (retval != PAPI_OK)
    LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",retval);
  thread_data = (papiex_perthread_data_t *)tmp;
  if (thread_data == NULL)
    {
      LIBPAPIEX_ERROR("PAPI_get_thr_specific() returned NULL.\n");
      return;
    }

  memset(&e,0,sizeof(e));
  tab = &thread_data->htab;
  if (tab == NULL)
    {
      LIBPAPIEX_DEBUG("Hash table not yet initialized.\n");
      return;
    }

  ultohexstr((unsigned long)this_fn,key);
  e.key = key;
  LIBPAPIEX_DEBUG("Looking up function 0x%s in hash table at %p.\n",e.key,tab);
  retval = hsearch_r(e,FIND,&ep,tab);
  if (retval)
    {
      if (ep == NULL)
	{
	  LIBPAPIEX_DEBUG("hsearch_r(FIND,%s) failed. %s\n",e.key,strerror(errno));
	  return;
	}
      point = (int)ep->data;
      LIBPAPIEX_DEBUG("Function 0x%s found, point %d.\n",e.key,point);
      papiex_stop(point);
    }
  else if ((retval == 0) && (errno != ESRCH))
    {
      LIBPAPIEX_ERROR("hsearch_t(FIND,%s) failed. %s\n",e.key,strerror(errno));
      return;
    }
  else
    {
      LIBPAPIEX_ERROR("Function 0x%s not found.\n",e.key);
      return;
    }
  }
}

void papiex_gcc_thread_shutdown(papiex_perthread_data_t *thread_data)
{
  LIBPAPIEX_DEBUG("%p\n",thread_data);
  if (thread_data->htab.size)
    {
      hdestroy_r(&thread_data->htab);
      LIBPAPIEX_DEBUG("Hash table of %d entries destroyed.\n",thread_data->max_caliper_entries);
    }
}

