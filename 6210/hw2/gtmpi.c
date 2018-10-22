#include <stdio.h>
#include <sys/utsname.h>
#include "mpi.h"
#include "gtmpi.h"


int main(int argc, char **argv)
{
  gtmpi_init(PROCESSORS);

  MPI_Init(&argc, &argv);

  int num_processes;
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  if (num_processes != PROCESSORS)
  {
    printf("Usage: mpiexec -n %d %s\n", PROCESSORS, argv[0]);
    printf("To change number of processors re-compile using: make %s MPI_PROCESSOR=<no. of processors>\n", argv[0][2]);
    return 1;
  }

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

  return 0;
}