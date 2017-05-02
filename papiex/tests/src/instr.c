#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

volatile double a = 0.1, b = 1.1, c = 2.1;

void one(void)
{
  int i;
  printf("Doing 1000000 iters. of a += b * c on doubles.\n");
  for (i=0;i<1000000;i++)
    a += b * c;
}

void two(void)
{
  int i;
  printf("Doing 10000000 iters. of a += b * c on doubles.\n");
  for (i=0;i<10000000;i++)
    a += b * c;
  one();
}

int main(int argc, char **argv)
{
  one();
  two();
  exit(0);
}

