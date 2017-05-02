#define PARAMS_MPI_Barrier (MPI_Comm comm)

typedef int (*MPI_Barrier_fptr_t) PARAMS_MPI_Barrier;

static MPI_Barrier_fptr_t real_MPI_Barrier = NULL;

#define PARAMS_MPI_Wait (MPI_Request *in_request, MPI_Status *status)

typedef int (*MPI_Wait_fptr_t) PARAMS_MPI_Wait;

static MPI_Wait_fptr_t real_MPI_Wait = NULL;

#define PARAMS_MPI_Waitall (int count, MPI_Request *in_array_of_requests, MPI_Status *array_of_statuses)

typedef int (*MPI_Waitall_fptr_t) PARAMS_MPI_Waitall;

static MPI_Waitall_fptr_t real_MPI_Waitall = NULL;

#define PARAMS_MPI_Waitany (int count, MPI_Request *in_array_of_requests, int *index, MPI_Status *status)

typedef int (*MPI_Waitany_fptr_t) PARAMS_MPI_Waitany;

static MPI_Waitany_fptr_t real_MPI_Waitany = NULL;

#define PARAMS_MPI_Waitsome (int count, MPI_Request *in_array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)

typedef int (*MPI_Waitsome_fptr_t) PARAMS_MPI_Waitsome;

static MPI_Waitsome_fptr_t real_MPI_Waitsome = NULL;

int MPI_Barrier PARAMS_MPI_Barrier
{
	papiex_perthread_data_t *thread = NULL;
	void *tmp = NULL;
	long long localstart = 0, localend = 0;
	int retval;
	if (papiex_mpiprof_gate)
	{
#ifdef HAVE_PAPI
		int ret = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
		if (ret != PAPI_OK)
		  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",ret);
		thread = (papiex_perthread_data_t *)tmp;
#endif
		localstart = PAPI_get_real_cyc();
	}

    if (!real_MPI_Barrier) {
	real_MPI_Barrier = (MPI_Barrier_fptr_t)dlsym(RTLD_NEXT,"MPI_Barrier"); 
 	handle_any_dlerror();
	if (real_MPI_Barrier == NULL) {
		abort();
	}
	retval = (*real_MPI_Barrier)(comm);
	} else
	retval = (*real_MPI_Barrier)(comm);
        localend = PAPI_get_real_cyc();
	if (thread != NULL && papiex_mpiprof_gate) { 
	  thread->papiex_mpiprof += localend - localstart;
	  thread->papiex_mpisyncprof += localend - localstart; 
	}

	return retval;
}

void mpi_barrier__(MPI_Comm *comm, MPI_Fint *ierr)
{
  *ierr = MPI_Barrier(*comm);
}

#pragma weak mpi_barrier = mpi_barrier__
#pragma weak mpi_barrier_ = mpi_barrier__
#pragma weak MPI_BARRIER = mpi_barrier__
#pragma weak MPI_BARRIER_ = mpi_barrier__
#pragma weak MPI_BARRIER__ = mpi_barrier__

int MPI_Wait PARAMS_MPI_Wait
{
	papiex_perthread_data_t *thread = NULL;
	void *tmp = NULL;
	long long localstart = 0, localend = 0;
	int retval;
	if (papiex_mpiprof_gate)
	{
#ifdef HAVE_PAPI
		int ret = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
		if (ret != PAPI_OK)
		  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",ret);
		thread = (papiex_perthread_data_t *)tmp;
#endif
		localstart = PAPI_get_real_cyc();
	}

    if (!real_MPI_Wait) {
	real_MPI_Wait = (MPI_Wait_fptr_t)dlsym(RTLD_NEXT,"MPI_Wait"); 
 	handle_any_dlerror();
	if (real_MPI_Wait == NULL) {
		abort();
	}
	retval = (*real_MPI_Wait)(in_request,status);
	} else
	retval = (*real_MPI_Wait)(in_request,status);
        localend = PAPI_get_real_cyc();
	if (thread != NULL && papiex_mpiprof_gate) { 
	  thread->papiex_mpiprof += localend - localstart;
	  thread->papiex_mpisyncprof += localend - localstart; 
	}

	return retval;
}

void mpi_wait__(MPI_Fint *req, MPI_Fint *stat, MPI_Fint *ierr)
{
  MPI_Request r;
  MPI_Status s;
  r = MPI_Request_f2c(*req);
  *ierr = MPI_Wait(&r, &s);
  MPI_Status_c2f(&s, stat);
  *req = MPI_Request_c2f(r);
}

#pragma weak mpi_wait = mpi_wait__
#pragma weak mpi_wait_ = mpi_wait__
#pragma weak MPI_WAIT = mpi_wait__
#pragma weak MPI_WAIT_ = mpi_wait__
#pragma weak MPI_WAIT__ = mpi_wait__

int MPI_Waitall PARAMS_MPI_Waitall
{
	papiex_perthread_data_t *thread = NULL;
	void *tmp = NULL;
	long long localstart = 0, localend = 0;
	int retval;
	if (papiex_mpiprof_gate)
	{
#ifdef HAVE_PAPI
		int ret = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
		if (ret != PAPI_OK)
		  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",ret);
		thread = (papiex_perthread_data_t *)tmp;
#endif
		localstart = PAPI_get_real_cyc();
	}

    if (!real_MPI_Waitall) {
	real_MPI_Waitall = (MPI_Waitall_fptr_t)dlsym(RTLD_NEXT,"MPI_Waitall"); 
 	handle_any_dlerror();
	if (real_MPI_Waitall == NULL) {
		abort();
	}
	retval = (*real_MPI_Waitall)(count,in_array_of_requests,array_of_statuses);
	} else
	retval = (*real_MPI_Waitall)(count,in_array_of_requests,array_of_statuses);
        localend = PAPI_get_real_cyc();
	if (thread != NULL && papiex_mpiprof_gate) { 
	  thread->papiex_mpiprof += localend - localstart;
	  thread->papiex_mpisyncprof += localend - localstart; 
	}

	return retval;
}

void mpi_waitall__ (int *count, MPI_Fint *in_array_of_requests, MPI_Fint array_of_statuses[][sizeof(MPI_Status)/sizeof(MPI_Fint)], MPI_Fint *ierr)
{
  int rc;
  MPI_Request *c_in_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request)*(*count));
  if (c_in_array_of_requests == NULL) {
    LIBPAPIEX_ERROR("Failed to allocate memory for %d MPI_Request's",*count); }
  { int i; for (i=0;i<*count;i++) {
    c_in_array_of_requests[i] = MPI_Request_f2c(in_array_of_requests[i]); } }
  MPI_Status *c_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status)*(*count));
  if (c_array_of_statuses == NULL) {
    LIBPAPIEX_ERROR("Failed to allocate memory for %d MPI_Status's",*count); }
  
  rc = MPI_Waitall(*count, c_in_array_of_requests, c_array_of_statuses);
  {
    int i; 
    for (i=0;i<*count;i++) {
      in_array_of_requests[i] = MPI_Request_c2f(c_in_array_of_requests[i]); 
      MPI_Status_c2f(&c_array_of_statuses[i], &array_of_statuses[i][0]);
    } 
  }
  free(c_array_of_statuses);
  free(c_in_array_of_requests);
  *ierr = (MPI_Fint)rc;
}

#pragma weak mpi_waitall = mpi_waitall__
#pragma weak mpi_waitall_ = mpi_waitall__
#pragma weak MPI_WAITALL = mpi_waitall__
#pragma weak MPI_WAITALL_ = mpi_waitall__
#pragma weak MPI_WAITALL__ = mpi_waitall__

int MPI_Waitany PARAMS_MPI_Waitany
{
	papiex_perthread_data_t *thread = NULL;
	void *tmp = NULL;
	long long localstart = 0, localend = 0;
	int retval;
	if (papiex_mpiprof_gate)
	{
#ifdef HAVE_PAPI
		int ret = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
		if (ret != PAPI_OK)
		  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",ret);
		thread = (papiex_perthread_data_t *)tmp;
#endif
		localstart = PAPI_get_real_cyc();
	}

    if (!real_MPI_Waitany) {
	real_MPI_Waitany = (MPI_Waitany_fptr_t)dlsym(RTLD_NEXT,"MPI_Waitany"); 
 	handle_any_dlerror();
	if (real_MPI_Waitany == NULL) {
		abort();
	}
	retval = (*real_MPI_Waitany)(count,in_array_of_requests,index,status);
	} else
	retval = (*real_MPI_Waitany)(count,in_array_of_requests,index,status);
        localend = PAPI_get_real_cyc();
	if (thread != NULL && papiex_mpiprof_gate) { 
	  thread->papiex_mpiprof += localend - localstart;
	  thread->papiex_mpisyncprof += localend - localstart; 
	}

	return retval;
}

void mpi_waitany__ (int *count, MPI_Fint *in_array_of_requests, int *index, MPI_Fint *status, MPI_Fint *ierr)
{
  int rc;
  MPI_Request *c_in_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request)*(*count));
  if (c_in_array_of_requests == NULL) {
    LIBPAPIEX_ERROR("Failed to allocate memory for %d MPI_Request's",*count); }
  { int i; for (i=0;i<*count;i++) {
    c_in_array_of_requests[i] = MPI_Request_f2c(in_array_of_requests[i]); } }
  MPI_Status c_status;
  rc = MPI_Waitany(*count, c_in_array_of_requests, index, &c_status);
  if (rc == MPI_SUCCESS) {
    if (*index >= 0) {
      in_array_of_requests[*index] = MPI_Request_c2f(c_in_array_of_requests[*index]); 
      (*index)++;
    }
    MPI_Status_c2f(&c_status, status);
  }
  free(c_in_array_of_requests);
  *ierr = (MPI_Fint)rc;
}

int MPI_Waitsome PARAMS_MPI_Waitsome
{
	papiex_perthread_data_t *thread = NULL;
	void *tmp = NULL;
	long long localstart = 0, localend = 0;
	int retval;
	if (papiex_mpiprof_gate)
	{
#ifdef HAVE_PAPI
		int ret = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);
		if (ret != PAPI_OK)
		  LIBPAPIEX_PAPI_ERROR("PAPI_get_thr_specific",ret);
		thread = (papiex_perthread_data_t *)tmp;
#endif
		localstart = PAPI_get_real_cyc();
	}

    if (!real_MPI_Waitsome) {
	real_MPI_Waitsome = (MPI_Waitsome_fptr_t)dlsym(RTLD_NEXT,"MPI_Waitsome"); 
 	handle_any_dlerror();
	if (real_MPI_Waitsome == NULL) {
		abort();
	}
	retval = (*real_MPI_Waitsome)(count,in_array_of_requests,outcount,array_of_indices,array_of_statuses);
	} else
	retval = (*real_MPI_Waitsome)(count,in_array_of_requests,outcount,array_of_indices,array_of_statuses);
        localend = PAPI_get_real_cyc();
	if (thread != NULL && papiex_mpiprof_gate) { 
	  thread->papiex_mpiprof += localend - localstart;
	  thread->papiex_mpisyncprof += localend - localstart; 
	}
	return retval;
}

void mpi_waitsome__ (int *count, MPI_Fint *in_array_of_requests, int *outcount, int *array_of_indices, MPI_Fint array_of_statuses[][sizeof(MPI_Status)/sizeof(MPI_Fint)], MPI_Fint *ierr)
{
  int rc;
  MPI_Request *c_in_array_of_requests = (MPI_Request*)malloc(sizeof(MPI_Request)*(*count));
  if (c_in_array_of_requests == NULL) {
    LIBPAPIEX_ERROR("Failed to allocate memory for %d MPI_Request's",*count); }
  { int i; for (i=0;i<*count;i++) {
    c_in_array_of_requests[i] = MPI_Request_f2c(in_array_of_requests[i]); } }
  MPI_Status *c_array_of_statuses = (MPI_Status*)malloc(sizeof(MPI_Status)*(*count));
  if (c_array_of_statuses == NULL) {
    LIBPAPIEX_ERROR("Failed to allocate memory for %d MPI_Status's",*count); }
  
  rc = MPI_Waitsome(*count, c_in_array_of_requests, outcount, array_of_indices, c_array_of_statuses);
  {  int i,j; 
    for (i=0;i<*count;i++) {
      if (i < *outcount) {
	if (array_of_indices[i] >= 0) {
	  in_array_of_requests[array_of_indices[i]] = MPI_Request_c2f(c_in_array_of_requests[array_of_indices[i]]); 
	} 
      } else {
	int found = j = 0;
	while ((!found) && (j<*outcount)) {
	  if (array_of_indices[j++] == i)
	    found = 1;
	}
	if (!found) 
	  in_array_of_requests[i] = MPI_Request_c2f(c_in_array_of_requests[i]);
      }
    }
    for (i=0;i<*outcount;i++)
      {
	MPI_Status_c2f(&c_array_of_statuses[i], &array_of_statuses[i][0]); 
	/* See the description of waitsome in the standard;
	   the Fortran index ranges are from 1, not zero */
	if (array_of_indices[i] >= 0) 
	  array_of_indices[i]++;
      }
  }
  free(c_array_of_statuses);
  free(c_in_array_of_requests);
  *ierr = (MPI_Fint)rc;
}

#pragma weak mpi_wait = mpi_wait__
#pragma weak mpi_wait_ = mpi_wait__
#pragma weak MPI_WAIT = mpi_wait__
#pragma weak MPI_WAIT_ = mpi_wait__
#pragma weak MPI_WAIT__ = mpi_wait__
#pragma weak mpi_waitall = mpi_waitall__
#pragma weak mpi_waitall_ = mpi_waitall__
#pragma weak MPI_WAITALL = mpi_waitall__
#pragma weak MPI_WAITALL_ = mpi_waitall__
#pragma weak MPI_WAITALL__ = mpi_waitall__
#pragma weak mpi_waitany = mpi_waitany__
#pragma weak mpi_waitany_ = mpi_waitany__
#pragma weak MPI_WAITANY = mpi_waitany__
#pragma weak MPI_WAITANY_ = mpi_waitany__
#pragma weak MPI_WAITANY__ = mpi_waitany__
#pragma weak mpi_waitsome = mpi_waitsome__
#pragma weak mpi_waitsome_ = mpi_waitsome__
#pragma weak MPI_WAITSOME = mpi_waitsome__
#pragma weak MPI_WAITSOME_ = mpi_waitsome__
#pragma weak MPI_WAITSOME__ = mpi_waitsome__
