#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

char zero_buf[1024*1024]; 
int main(int argc, char **argv) {
  int i;
  unlink("zero");
  FILE* myfile = fopen("zero", "w+");
  if (argc <3) {
    fprintf(stderr, "usage: ./writer <write> <stride>\n"
                    "where <write> is number of MB to write, and <stride> is seek between reads\n");
    return(1);
  }
  int num= atoi(argv[1]);
  int stride = atoi(argv[2]);
  for (i=0;i<num; i++) {
    fwrite(zero_buf, 1024*1024, 1, myfile);
  } 
  fflush(myfile);
  rewind(myfile);
  int fd = fileno(myfile);
  for (i=0; i<num; i+=stride+1) {
    fread(zero_buf, 1024*1024, 1, myfile);
    lseek(fd, stride * 1024 * 1024, SEEK_CUR);
    stride++;
  }
  fclose(myfile);
  exit(0);
}
