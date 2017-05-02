#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
  printf("Parent %d, fork().\n",getpid());
  if (fork() == 0)
    { 
      printf("execlp(basic,basic,NULL)\n");
      exit(execlp("./basic","basic", NULL));
    }
  else
    { 
      int stat;
      wait(&stat);
      exit(WEXITSTATUS(stat));
    }
}
