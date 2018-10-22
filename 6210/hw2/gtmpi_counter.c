#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/** Things changed to optimize:
 *  1. Have one master node, allowing all other nodes to signal it
 *  2. Have the master node signal all other nodes once it has heard from all the others
 *     These changes allow for the O(N^2) messages to become O(N) messages
 *  3. Allocate a single static status variable to save some space [O(1) instead of O(N) space]
 */

static int count;

void gtmpi_init(int n_threads)
{
  count = n_threads;
}

void gtmpi_barrier()
{
  static MPI_Status status;

  int vpid;
  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);
  printf("Entering %d\n", vpid);
  fflush(stdout);
  
  if (vpid == 0)
  {
    for(int i = 1; i < count; i++)
    {
      MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
      printf("0 recevied message from %d\n", i);
      fflush(stdout);
    }

    for(int i = 1; i < count; i++)
    {
      MPI_Send(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD);
      printf("0 sent message to %d\n", i);
      fflush(stdout);
    }
  }
  else
  {
    MPI_Send(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD);
    printf("%d sent message to 0\n", vpid);
    fflush(stdout);
    MPI_Recv(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    printf("%d received message from 0\n", vpid);
    fflush(stdout);
  }

  printf("Leaving %d\n", vpid);
  fflush(stdout);
}

void gtmpi_finalize()
{
}