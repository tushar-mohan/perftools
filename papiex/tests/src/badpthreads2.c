#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

volatile double a = 0.1, b = 1.1, c = 2.1;

void *Thread(void *arg)
{
  int i, n = *(int *)arg;
  printf("PThread Thread 0x%x: %d iterations\n",(unsigned)pthread_self(),n);
  for (i=0;i<n;i++)
    a += b * c;
  return(NULL);
}

int main(int argc, char **argv)
{
   pthread_t e_th;
   pthread_t f_th;
   int flops1, flops2;
   int retval;
   pthread_attr_t attr;

   pthread_attr_init(&attr);
#ifdef PTHREAD_CREATE_UNDETACHED
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_UNDETACHED);
#endif
#ifdef PTHREAD_SCOPE_SYSTEM
   retval = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
   if (retval != 0)
     {
       fprintf(stderr,"This system does not support kernel scheduled pthreads.\n");
       exit(1);
     }
#endif

   flops1 = 1000000;
   pthread_create(&e_th, &attr, Thread, (void *) &flops1);
   flops2 = 2000000;
   pthread_create(&f_th, &attr, Thread, (void *) &flops2);
   pthread_attr_destroy(&attr);

   _exit(0);
}
