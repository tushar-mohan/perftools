#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "papiex.h"

volatile double a = 0.1, b = 1.1, c = 2.1;

int main(int argc, char **argv)
{
  int i;

#if defined(FULL_CALIPER)
  PAPIEX_START_ARG(1,"printf");
#endif

  printf("Doing 100000000 iters. of a += b * c on doubles.\n");

#if defined(FULL_CALIPER)
  PAPIEX_STOP_ARG(1);
#endif

#if !defined(FULL_CALIPER)
  PAPIEX_START();
#else
  PAPIEX_START_ARG(2,"for loop");
#endif

  for (i=0;i<100000000;i++)
    a += b * c;

#if !defined(FULL_CALIPER)
  PAPIEX_STOP();
#else
  PAPIEX_STOP_ARG(2);
#endif

  exit(0);
}
