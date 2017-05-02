#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

volatile double a = 0.1, b = 1.1, c = 2.1;

int main(int argc, char **argv)
{
  int rc = write(1,"Done.\n",sizeof("Done.\n"));
  exit(rc);
}
