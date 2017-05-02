#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

char zero_buf[1024*1024]; 
int main(int argc, char **argv) {
  int i, sync;
  sync=0;
  if (getenv("WRITE_SYNC")) {
    sync=1;
    fprintf(stderr, "In synchronous mode\n");
  }
  unlink("zero");
  FILE* myfile = fopen("zero", "w");
  if (argc <2) {
    fprintf(stderr, "usage: ./writer <num>, where num is an integer giving number of MB to write\n");
    return(1);
  }
  int num= atoi(argv[1]);
  for (i=0;i<num; i++) {
    fwrite(zero_buf, 1024*1024, 1, myfile);
    if (sync) fflush(myfile);
  } 
  fclose(myfile);
  exit(0);
}
