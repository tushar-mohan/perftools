/* This file performs the following test: start, stop and timer
functionality for 2 slave pthreads */

#include <stdio.h>

#ifdef USE_MPI
#  include <mpi.h>
#endif

#ifdef HAVE_PTHREADS
#  include <pthread.h>
#endif


void flops(int count)
{
   double a = 1.01;
   int i;
   for (i=0; i< count;i++) {
     a = a * 1.1;
     if (a > 10000)
       a = 1.01;
   }
   printf("a=%f\n", a);
   
}

#ifdef HAVE_PTHREADS
void *thr_flops(void *arg) {
  flops(*((int*)(arg)));
  return ((void *)NULL);
}
#endif

int main(int argc, char **argv)
{
#ifdef USE_MPI
   MPI_Init(&argc, &argv);
#endif

   int  flops1, flops2, flops3, flops4;
   flops1 = 10000000 ;
   flops2 = 40000000 ;
   flops3 = 40000000 ;
   flops4 = 20000000 ;

#ifdef HAVE_PTHREADS
   pthread_t e_th, f_th, g_th;
   int retval, rc;
   pthread_attr_t attr;

   pthread_attr_init(&attr);
#ifdef PTHREAD_CREATE_UNDETACHED
     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_UNDETACHED);
#endif
#ifdef PTHREAD_SCOPE_SYSTEM
     retval = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
#endif

   rc = pthread_create(&e_th, &attr, thr_flops, (void *) &flops1);
   rc = pthread_create(&f_th, &attr, thr_flops, (void *) &flops2);
   rc = pthread_create(&g_th, &attr, thr_flops, (void *) &flops3);

#else
   flops(flops1);
   flops(flops2);
   flops(flops3);
#endif /* HAVE_PTHREADS */
   flops(flops4);

#ifdef HAVE_PTHREADS
   pthread_join(e_th, NULL);
   pthread_join(f_th, NULL);
   pthread_join(g_th, NULL);
#endif

#ifdef USE_MPI
   MPI_Finalize();
#endif
   return 0;
}
