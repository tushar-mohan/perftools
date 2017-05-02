#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "papiex.h"

volatile double a = 0.1, b = 1.1, c = 2.1;

int main(int argc, char **argv)
{
  int i;

  printf("Doing 2*100000000 iters. of a += b * c on doubles.\n");

  PAPIEX_START();

  for (i=0;i<100000000;i++)
    a += b * c;

  PAPIEX_STOP();

  PAPIEX_START();

  for (i=0;i<100000000;i++)
    a += b * c;

  PAPIEX_STOP();

  exit(0);
}
