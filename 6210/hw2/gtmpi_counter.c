#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
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
  fflush(stdout);
  
  if (vpid == 0)
  {
    for(int i = 1; i < count; i++)
    {
      printf("%lu 0 receving message from %d\n", (unsigned long)time(NULL), i);
      fflush(stdout);
      MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
      printf("%lu 0 recevied message from %d\n", (unsigned long)time(NULL), i);
      fflush(stdout);
    }

    for(int i = 1; i < count; i++)
    {
      printf("%lu 0 sending message to %d\n", (unsigned long)time(NULL), i);
      fflush(stdout);
      MPI_Send(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD);
      printf("%lu 0 sent message to %d\n", (unsigned long)time(NULL), i);
      fflush(stdout);
    }
  }
  else
  {
    printf("%lu %d sending message to 0\n", (unsigned long)time(NULL), vpid);
    fflush(stdout);
    MPI_Send(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD);
    printf("%lu %d sent message to 0\n", (unsigned long)time(NULL), vpid);
    fflush(stdout);
    printf("%lu %d receiving message from 0\n", (unsigned long)time(NULL), vpid);
    fflush(stdout);
    MPI_Recv(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    printf("%lu %d received message from 0\n", (unsigned long)time(NULL), vpid);
    fflush(stdout);
  }

  fflush(stdout);
}

void gtmpi_finalize()
{
}