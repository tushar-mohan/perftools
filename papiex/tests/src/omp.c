#ifdef _OPENMP
#include <omp.h>
#else
#error "This compiler does not understand OPENMP"
#endif
#include <stdlib.h>
#include <stdio.h>

volatile double a = 0.1, b = 1.1, c = 2.1;

void Thread(int n)
{
  int i;
  printf("OpenMP Thread %d: %d iterations\n",n-1,n*1000000);
  for (i=0;i<n*100000;i++)
    a += b * c;
}

int main(int argc, char **argv)
{
#pragma omp parallel
   {
      Thread(omp_get_thread_num()+1);
#pragma omp barrier
   }

   exit(0);
}
