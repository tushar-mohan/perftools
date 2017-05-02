#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
  printf("Parent %d, fork().\n",getpid());
  if (fork() == 0)
    { 
      char pwd[PATH_MAX];
      if (getcwd(pwd,PATH_MAX) == NULL) {
        fprintf(stderr, "error getting working directory");
        exit(1);
      }
      strcat(pwd,"/basic");
      printf("execl(%s,%s,NULL)\n",pwd,pwd);
      exit(execl(pwd,pwd,NULL));
    }
  else
    { 
      int stat;
      wait(&stat);
      exit(WEXITSTATUS(stat));
    }
}
