#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define ITERS 1000000000

volatile double a = 0.1, b = 1.1, c = 2.1;

int main(int argc, char **argv)
{
  int i;
  printf("Doing %d iters. of a += b * c on doubles.\n",ITERS);
  for (i=0;i<ITERS;i++)
    a += b * c;
  exit(0);
}
