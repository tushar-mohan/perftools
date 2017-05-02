#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define ITERS 100000000

volatile double a = 0.1, b = 1.1, c = 2.1;

int main(int argc, char **argv)
{
  int i;
  printf("Parent %d, fork().\n",getpid());
  if (fork() == 0)
    { 
      printf("Child %d, %d multiply-adds.\n",getpid(),2*ITERS);
      for (i=0;i<2*ITERS;i++)
	a += b * c;
      exit(0);
    }
  else
    { 
      int stat; 
      printf("Parent %d, %d multiply-adds.\n",getpid(),ITERS);
      for (i=0;i<ITERS;i++)
	a += b * c;
      wait(&stat);
      exit(WEXITSTATUS(stat));
    }
}
