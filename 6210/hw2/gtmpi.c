#include <stdio.h>
#include <sys/utsname.h>
#include "mpi.h"
#include "gtmpi.h"


int main(int argc, char **argv)
{
  printf("This is the serial section\n");

  int num_processes;
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  gtmpi_init(num_processes);

  MPI_Init(&argc, &argv);

  int my_id;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

  struct utsname ugnm;
  uname(&ugnm);

  printf("Hello World from thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);
  gtmpi_barrier();

  printf("Goodbye world from thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);
  gtmpi_barrier();

  MPI_Finalize();

  gtmpi_finalize();

  printf("Back in the serial section again\n");
  return 0;
}

