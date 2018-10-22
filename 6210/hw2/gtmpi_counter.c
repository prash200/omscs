#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

/** Things changed to optimize:
 *  1. Have one master node, allowing all other nodes to signal it
 *  2. Have the master node signal all other nodes once it has heard from all the others
 *     These changes allow for the O(N^2) messages to become O(N) messages
 *  3. Allocate a single static status variable to save some space [O(1) instead of O(N) space]
 */

static int P;

void gtmpi_init(int num_threads)
{
  P = num_threads;
}

void gtmpi_barrier()
{
  static MPI_Status status;
  int vpid, i;

  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);
  
  if (vpid == 0)
  {
    for(i = 1; i < P; i++)
    {
      MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
    }

    for(i = 1; i < P; i++)
    {
      MPI_Send(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD);
    }
  }
  else
  {
    MPI_Send(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD);
    MPI_Recv(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
  }
}

void gtmpi_finalize()
{
}