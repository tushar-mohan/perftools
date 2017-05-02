#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
  char pwd[PATH_MAX];
  if (getcwd(pwd,PATH_MAX) == NULL) {
    fprintf(stderr, "error getting working directory");
    exit(1);
  }
  strcat(pwd,"/basic");
  printf("system(%s)\n",pwd);
  exit(system(pwd));
}
