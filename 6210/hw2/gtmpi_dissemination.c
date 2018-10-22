#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

static unsigned int num_procs;
static unsigned int num_rounds;
static unsigned int cur_bar;
static unsigned int sense_init;
static unsigned int parity_init;

static int ceillog2(int val) 
{
  int i = 0;
  while (val >> i != 1)
  {
    i++;
  }

  return ((1 << i) == val) ? i : i+1;
}

void gtmpi_init(int n_procs)
{
  num_procs = n_procs;
  num_rounds = ceillog2(num_procs);
  sense_init = 1;
  parity_init = 0;
  cur_bar = 0;
}

void gtmpi_barrier(){
  int parity = parity_init;
  char sense = sense_init;
  int bar = cur_bar;
  int vpid; 
  int i, src, dst;
  MPI_Status stat;

  // MPI controls the IDs that we need to use
  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);

  for (i = 0; i < num_rounds; i++) {
    // do MPI send here
    dst = (vpid + (1<<i)) % num_procs;
    MPI_Send(&sense, 1, MPI_CHAR, dst, (i + (bar * num_rounds)), MPI_COMM_WORLD);

    // do MPI recv here
    src = ((vpid - (1<<i)) + num_procs) % num_procs;
    MPI_Recv(&sense, 1, MPI_CHAR, src, (i + (bar * num_rounds)), MPI_COMM_WORLD, &stat);
  }

  if (parity == 1) {
    sense_init = !sense_init;
  }

  parity_init = (parity + 1) % 2;
  cur_bar++;
}

void gtmpi_finalize(){
}
