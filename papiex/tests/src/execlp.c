#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
  printf("execlp(basic,basic,NULL)\n");
  exit(execlp("basic","basic",NULL));
}
