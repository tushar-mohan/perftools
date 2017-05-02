#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  MPI_File fh;
  MPI_Status status;
  int rank, nprocs, nints;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  
  if (argc != 2) {
    fprintf(stderr, "usage: mpio_write <file size in MB>\n");
    exit(1);
  } 

  int filesize = atoi(argv[1]) * 1024 * 1024; 
  int bufsize = filesize/nprocs;

  char * buf = malloc(bufsize);

  if (buf == NULL) {
    fprintf(stderr, "Malloc of %d bytes failed\n", bufsize);
    exit(2);
  }

  nints = bufsize/sizeof(int);
  memset(buf, 0, bufsize);

  MPI_File_open(MPI_COMM_WORLD, "./zero", 
                MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, rank * bufsize, MPI_SEEK_SET);
  MPI_File_write(fh, buf, nints, MPI_INT, &status);
  MPI_File_close(&fh);

  /* read */
  MPI_File_open(MPI_COMM_WORLD, "./zero", 
                MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, rank * bufsize, MPI_SEEK_SET);
  MPI_File_read(fh, buf, nints, MPI_INT, &status);
  MPI_File_close(&fh);
  MPI_Finalize();
  return 0;
}
