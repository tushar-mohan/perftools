/* This file performs the following test: start, stop and timer
functionality for 2 slave pthreads */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

char zero_buf[1024*1024]; 
int num;

FILE *glob_file = NULL;
void *Thread(void *arg)
{
  int i;
  char file[1024];
  unsigned long n = (unsigned long) (arg);
  sprintf(file,"zero_%lu", n);
  unlink(file);
  FILE* myfile = fopen(file, "w");
  for (i=0;i<num; i++) {
    fwrite(zero_buf, 1024*1024, 1, myfile);
    fwrite(zero_buf, 1024*1024, 1, glob_file);
  } 
  fflush(myfile);
  fclose(myfile);
  return (void*)0;
}

int main(int argc, char **argv)
{
   pthread_t x_th[10];
   unsigned long i;
   int rc;
   pthread_attr_t attr;

   pthread_attr_init(&attr);
#ifdef PTHREAD_CREATE_UNDETACHED
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_UNDETACHED);
#endif
#ifdef PTHREAD_SCOPE_SYSTEM
   pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
#endif

  if (argc <2) {
    fprintf(stderr, "usage: ./writer <num>, where num is an integer giving number of MB to write\n");
    return(1);
  }
  num= atoi(argv[1]);
  glob_file = fopen("zero", "a");

  for (i=0; i<4; i++) {
     rc = pthread_create(&x_th[i], &attr, Thread, (void*)i);
     if (rc<0) {
       perror("Could not create thread");
     } 
  }

   Thread((void*)i);
   for (i=0; i< 4; i++)
     pthread_join(x_th[i], NULL);

   pthread_attr_destroy(&attr);
   pthread_exit(NULL);
}
