#include <stdlib.h>
#include <mpi.h>
#include "gtmpi.h"

static int count, num_rounds;

static int ceillog2(int val)
{
  int i = 0;
  while (val >> i != 1)
  {
    i++;
  }

  return ((1 << i) == val) ? i : i + 1;
}

void gtmpi_init(int n_threads)
{
  count = n_threads;
  num_rounds = ceillog2(count);
}

void gtmpi_barrier()
{
  static MPI_Status status;

  int vpid;
  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);

  for (int i = 0; i < num_rounds; i++)
  {
    int destination_vpid = (vpid + (1 << i)) % count;
    MPI_Send(NULL, 0, MPI_INT, destination_vpid, 1, MPI_COMM_WORLD);

    int source_vpid = ((vpid - (1 << i)) + count) % count;
    MPI_Recv(NULL, 0, MPI_INT, source_vpid, 1, MPI_COMM_WORLD, &status);
  }
}

void gtmpi_finalize()
{
}